#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct stock_t
{
  char name[256];
  char measureUnit[256];
  int originalQuantity;
  int quantity;
  int unitPrice;
  pthread_mutex_t mutex;
} stock_t;

typedef struct bill_item_t
{
  stock_t *stock;
  int saleQuantity;
  int salePrice;
} bill_item_t;

typedef struct bill_t
{
  bill_item_t **items;
  int count;
  int totalSalePrice;
  int isFinished;
} bill_t;

typedef struct money_t
{
  int money;
  pthread_mutex_t mutex;
} money_t;

typedef struct thread_payload_t
{
  bill_t **bills;
  int billCount;
  stock_t **stocks;
  int stockCount;
  bill_t **performedBills;
  int performedBillCount;
  money_t *money;

  pthread_mutex_t billMutex;
  pthread_mutex_t performedBillMutex;

} thread_payload_t;

stock_t *create_stock(const char *name, int unitPrice, const char *measureUnit, int quantity)
{
  stock_t *stock = malloc(sizeof(stock_t));
  strcpy(stock->name, name);
  strcpy(stock->measureUnit, measureUnit);
  stock->quantity = quantity;
  stock->originalQuantity = quantity;
  stock->unitPrice = unitPrice;
  pthread_mutex_init(&stock->mutex, NULL);
  return stock;
}

bill_t *create_bill(bill_item_t **items, int count)
{
  int totalSalePrice = 0;
  for (int i = 0; i < count; i++)
  {
    totalSalePrice += items[i]->salePrice;
  }
  bill_t *bill = malloc(sizeof(bill_t));
  bill->items = items;
  bill->totalSalePrice = totalSalePrice;
  bill->count = count;
  bill->isFinished = 0;
  return bill;
}

bill_item_t *create_bill_item(stock_t *stock, int saleQuantity)
{
  int unitPrice = stock->unitPrice;
  int salePrice = saleQuantity * unitPrice;
  bill_item_t *billItem = malloc(sizeof(bill_item_t));
  billItem->salePrice = salePrice;
  billItem->saleQuantity = saleQuantity;
  billItem->stock = stock;
  return billItem;
}

money_t *create_money(int money)
{
  money_t *mon = malloc(sizeof(money_t));
  mon->money = money;
  pthread_mutex_init(&mon->mutex, NULL);
  return mon;
}

int inventory_check(bill_t **bills, int count, stock_t **stocks, int stockCount, money_t *money)
{
  int total = 0;
  for (int i = 0; i < count; i++)
  {
    if (!bills[i]->isFinished)
    {
      continue;
    }
    total += bills[i]->totalSalePrice;
  }
  int stockTotal = 0;
  for (int i = 0; i < stockCount; i++)
  {
    stock_t *stock = stocks[i];
    pthread_mutex_lock(&stock->mutex);
    stockTotal += (stock->originalQuantity - stock->quantity) * stock->unitPrice;
    pthread_mutex_unlock(&stock->mutex);
  }
  int cond1 = total == money->money;
  int cond2 = stockTotal == total;
  if (cond1 && cond2)
  {
    printf("INVENTORY CHECK: \e[1;32mPASSED\e[0m\n");
  }
  else
  {
    printf("INVENTORY CHECK: \e[1;31mFAILED\e[0m %d&%d\n", cond1, cond2);
  }
}

void *thread_inv_check(void *voidCtx)
{
  thread_payload_t *context = (thread_payload_t *)voidCtx;
  while (1)
  {
    pthread_mutex_lock(&context->performedBillMutex);
    pthread_mutex_lock(&context->money->mutex);
    inventory_check(context->performedBills, context->performedBillCount, context->stocks, context->stockCount, context->money);
    if (context->billCount == 0)
    {
      printf("Exiting check thread\n");
      pthread_mutex_unlock(&context->performedBillMutex);
      pthread_mutex_unlock(&context->money->mutex);
      return NULL;
    }
    pthread_mutex_unlock(&context->performedBillMutex);
    pthread_mutex_unlock(&context->money->mutex);
    sleep(1);
  }
}

void *thread_do_bills(void *voidCtx)
{
  thread_payload_t *ctx = (thread_payload_t *)voidCtx;
  // perform last bill on the "stack" of bills
  while (1)
  {
    pthread_mutex_lock(&ctx->billMutex);
    if (ctx->billCount == 0)
    {
      printf("Exiting processor thread\n");
      pthread_mutex_unlock(&ctx->billMutex);
      return NULL;
    }
    int idx = --ctx->billCount;
    bill_t *bill = ctx->bills[idx];
    if (idx % 100 == 0)
      printf("Starting processing bill #%d\n", idx);
    pthread_mutex_unlock(&ctx->billMutex);
    int money = 0;
    for (int i = 0; i < bill->count; i++)
    {
      bill_item_t *item = bill->items[i];
      pthread_mutex_lock(&item->stock->mutex);
      money += item->salePrice;
      // printf("Processed stock item %s\n", item->stock->name);
      pthread_mutex_unlock(&item->stock->mutex);
    }
    pthread_mutex_lock(&ctx->performedBillMutex);
    pthread_mutex_lock(&ctx->money->mutex);
    ctx->performedBills[ctx->performedBillCount++] = bill;
    bill->isFinished = 1;
    ctx->money->money += money;
    // decrement quantities on all products
    for (int i = 0; i < bill->count; i++)
    {
      bill_item_t *item = bill->items[i];
      pthread_mutex_lock(&item->stock->mutex);
      item->stock->quantity -= item->saleQuantity;
      pthread_mutex_unlock(&item->stock->mutex);
    }
    if (idx % 100 == 0)
      printf("Finished processing bill #%d\n", idx);
    pthread_mutex_unlock(&ctx->money->mutex);
    pthread_mutex_unlock(&ctx->performedBillMutex);
  }
  return NULL;
}

int main()
{
  // generate start condition
  srand(time(NULL));
  printf("Generating stocks\n");
  stock_t *milk = create_stock("Milk", 450, "l", 154572);
  stock_t *bread = create_stock("Bread", 600, "pcs", 264654);
  stock_t *chocolate = create_stock("Chocolate", 1200, "pcs", 98);
  stock_t *stocks[] = {milk, bread, chocolate};
  const int stockCount = sizeof(stocks) / sizeof(stock_t *);

  printf("Generating bills\n");
  const int billCount = 10000;
  bill_t **bills = malloc(sizeof(bill_t *) * billCount);
  for (int i = 0; i < billCount; i++)
  {
    int itemsCount = rand() % 10000 + 5;
    // int itemsCount = 2;
    bill_item_t **items = malloc(sizeof(bill_item_t *) * itemsCount);
    for (int j = 0; j < itemsCount; j++)
    {
      stock_t *randomStock = stocks[rand() % stockCount];
      int randomQuantity = rand() % 10 + 1;
      bill_item_t *item = create_bill_item(randomStock, randomQuantity);
      items[j] = item;
    }
    bills[i] = create_bill(items, itemsCount);
  }

  // print status
  printf("Stock count %d\n", stockCount);
  for (int i = 0; i < stockCount; i++)
  {
    stock_t *s = stocks[i];
    // printf("%15s: %5.2fRON x%4d %-2s\n", s->name, s->unitPrice / 100., s->quantity, s->measureUnit);
  }
  printf("Bills count %d\n", billCount);
  for (int i = 0; i < billCount; i++)
  {
    bill_t *bill = bills[i];
    // printf("- bill with %d items worth %5.2fRON\n", bill->count, bill->totalSalePrice / 100.);
    for (int j = 0; j < bill->count; j++)
    {
      bill_item_t *item = bill->items[j];
      // printf("  %4dx %-15s worth %5.2fRON\n", item->saleQuantity, item->stock->name, item->salePrice / 100.);
    }
  }

  // start to play around now
  money_t *money = create_money(0);

  printf("Making thread context\n");
  thread_payload_t *context = malloc(sizeof(thread_payload_t));
  context->bills = bills;
  context->billCount = billCount;
  context->performedBillCount = 0;
  context->performedBills = malloc(sizeof(bill_t *) * billCount);
  context->stocks = stocks;
  context->stockCount = stockCount;
  context->money = money;
  pthread_mutex_init(&context->billMutex, NULL);
  pthread_mutex_init(&context->performedBillMutex, NULL);

  printf("Allocating threads\n");
  pthread_t *invCheckThread = malloc(sizeof(pthread_t));
  pthread_create(invCheckThread, NULL, thread_inv_check, (void *)context);
  sleep(1);
  int processingCount = 10;
  pthread_t **processingThreads = malloc(sizeof(pthread_t *) * processingCount);
  printf("Creating threads\n");
  for (int i = 0; i < processingCount; i++)
  {
    processingThreads[i] = malloc(sizeof(pthread_t));
    pthread_create(processingThreads[i], NULL, thread_do_bills, (void *)context);
  }
  for (int i = 0; i < processingCount; i++)
  {
    pthread_join(*processingThreads[i], NULL);
  }
  pthread_join(*invCheckThread, NULL);
  // thread_inv_check((void *)context);
  printf("Finishing\n");

  // deallocate everything
  for (int i = 0; i < stockCount; i++)
  {
    stock_t *s = stocks[i];
    free(s);
  }
  for (int i = 0; i < billCount; i++)
  {
    bill_t *bill = bills[i];
    for (int j = 0; j < bill->count; j++)
    {
      bill_item_t *item = bill->items[j];
      free(item);
    }
    free(bill->items);
    free(bill);
  }
  free(bills);
  free(money);
  free(context->performedBills);
  free(context);
  free(invCheckThread);
  for (int i = 0; i < processingCount; i++)
  {
    free(processingThreads[i]);
  }
  free(processingThreads);
}