#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "Psapi.lib")
#endif
#else
#include <sys/resource.h>
#endif

#define BIG_SIZE 100000
#define SMALL_SIZE 1000
#define RUNS 5

/* Every sorting function returns 1 on success and 0 on failure. */
typedef int (*SortFunction)(int[], int);

/* 1. GENERATE RANDOM ARRAY */
void generateArray(int a[], int n)
{
    for (int i = 0; i < n; i++)
        a[i] = rand() % 100000;
}

/* 2. BUBBLE SORT */
int bubbleSort(int a[], int n)
{
    if (a == NULL || n < 0)
        return 0;

    for (int i = 0; i < n - 1; i++)
    {
        int swapped = 0;

        for (int j = 0; j < n - i - 1; j++)
        {
            if (a[j] > a[j + 1])
            {
                int temp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = temp;
                swapped = 1;
            }
        }

        if (!swapped)
            break;
    }
    return 1;
}

/* 3. SELECTION SORT */
int selectionSort(int a[], int n)
{
    if (a == NULL || n < 0)
        return 0;

    for (int i = 0; i < n - 1; i++)
    {
        int minIndex = i;

        for (int j = i + 1; j < n; j++)
        {
            if (a[j] < a[minIndex])
                minIndex = j;
        }

        if (minIndex != i)
        {
            int temp = a[i];
            a[i] = a[minIndex];
            a[minIndex] = temp;
        }
    }
    return 1;
}

/* 4. INSERTION SORT */
int insertionSort(int a[], int n)
{
    if (a == NULL || n < 0)
        return 0;

    for (int i = 1; i < n; i++)
    {
        int key = a[i];
        int j = i - 1;

        while (j >= 0 && a[j] > key)
        {
            a[j + 1] = a[j];
            j--;
        }

        a[j + 1] = key;
    }
    return 1;
}

/* 5. MERGE SORT */
static void merge(int a[], int left, int mid, int right, int temp[])
{
    int i = left;
    int j = mid + 1;
    int k = left;

    while (i <= mid && j <= right)
    {
        if (a[i] <= a[j])
            temp[k++] = a[i++];
        else
            temp[k++] = a[j++];
    }

    while (i <= mid)
        temp[k++] = a[i++];

    while (j <= right)
        temp[k++] = a[j++];

    for (i = left; i <= right; i++)
        a[i] = temp[i];
}

static void mergeSortRec(int a[], int left, int right, int temp[])
{
    if (left >= right)
        return;

    int mid = left + (right - left) / 2;

    mergeSortRec(a, left, mid, temp);
    mergeSortRec(a, mid + 1, right, temp);
    merge(a, left, mid, right, temp);
}

int mergeSort(int a[], int n)
{
    if (a == NULL || n < 0)
        return 0;
    if (n <= 1)
        return 1;

    if ((size_t)n > (size_t)-1 / sizeof(int))
        return 0;

    int *temp = malloc((size_t)n * sizeof(int));
    if (temp == NULL)
        return 0;

    mergeSortRec(a, 0, n - 1, temp);
    free(temp);
    return 1;
}

/* 6. QUICK SORT (Hoare partition) */
static int partition(int a[], int low, int high)
{
    int pivot = a[low + (high - low) / 2];
    int i = low - 1;
    int j = high + 1;

    while (1)
    {
        do
        {
            i++;
        } while (a[i] < pivot);

        do
        {
            j--;
        } while (a[j] > pivot);

        if (i >= j)
            return j;

        int temp = a[i];
        a[i] = a[j];
        a[j] = temp;
    }
}

static void quickSortRec(int a[], int low, int high)
{
    while (low < high)
    {
        int p = partition(a, low, high);

        /* Recurse into the smaller side to limit recursion depth. */
        if (p - low < high - (p + 1))
        {
            quickSortRec(a, low, p);
            low = p + 1;
        }
        else
        {
            quickSortRec(a, p + 1, high);
            high = p;
        }
    }
}

int quickSort(int a[], int n)
{
    if (a == NULL || n < 0)
        return 0;
    if (n <= 1)
        return 1;

    quickSortRec(a, 0, n - 1);
    return 1;
}

/* 7. CHECK SORT ORDER AND EXACT RESULT */
static int compareInts(const void *x, const void *y)
{
    int a = *(const int *)x;
    int b = *(const int *)y;
    return (a > b) - (a < b);
}

static int arraysEqual(const int a[], const int b[], int n)
{
    for (int i = 0; i < n; i++)
    {
        if (a[i] != b[i])
            return 0;
    }
    return 1;
}

/* 8. MEMORY MEASUREMENT
   This is the peak memory of the whole process, not per algorithm. */
double getMemoryUsage(void)
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    memset(&pmc, 0, sizeof(pmc));
    pmc.cb = sizeof(pmc);

    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return (double)pmc.PeakWorkingSetSize / (1024.0 * 1024.0);
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0)
    {
#ifdef __APPLE__
        return (double)usage.ru_maxrss / (1024.0 * 1024.0);
#else
        return (double)usage.ru_maxrss / 1024.0;
#endif
    }
#endif
    return -1.0;
}

/* 9. RUN ONE ALGORITHM */
void runOne(SortFunction sort, const char *name,
            const int original[], int n)
{
    if (sort == NULL || name == NULL || original == NULL || n <= 0)
    {
        fprintf(stderr, "Invalid arguments passed to runOne().\n");
        return;
    }

    if ((size_t)n > (size_t)-1 / sizeof(int))
    {
        fprintf(stderr, "Array size is too large.\n");
        return;
    }

    int *a = malloc((size_t)n * sizeof(int));
    int *expected = malloc((size_t)n * sizeof(int));

    if (a == NULL || expected == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        free(a);
        free(expected);
        return;
    }

    memcpy(expected, original, (size_t)n * sizeof(int));
    qsort(expected, (size_t)n, sizeof(int), compareInts);

    double totalTime = 0.0;
    int correct = 1;

    for (int r = 0; r < RUNS; r++)
    {
        memcpy(a, original, (size_t)n * sizeof(int));

        clock_t start = clock();
        if (start == (clock_t)-1)
        {
            fprintf(stderr, "Could not start execution-time measurement.\n");
            free(a);
            free(expected);
            return;
        }

        int sortOK = sort(a, n);

        clock_t end = clock();
        if (end == (clock_t)-1)
        {
            fprintf(stderr, "Could not finish execution-time measurement.\n");
            free(a);
            free(expected);
            return;
        }

        if (!sortOK)
        {
            fprintf(stderr, "%s failed (possibly not enough memory).\n", name);
            free(a);
            free(expected);
            return;
        }

        totalTime += (double)(end - start) / CLOCKS_PER_SEC;

        /* Compare with qsort to detect both ordering and data-loss bugs. */
        if (!arraysEqual(a, expected, n))
            correct = 0;
    }

    printf("\n------------------------------\n");
    printf("Algorithm: %s\n", name);
    printf("Array size: %d\n", n);
    printf("Average time: %.6f seconds\n", totalTime / RUNS);

    double memory = getMemoryUsage();
    if (memory >= 0.0)
        printf("Peak process memory: %.2f MB\n", memory);
    else
        printf("Peak process memory: unavailable\n");

    if (correct)
        printf("Result: SUCCESS - Correctly sorted.\n");
    else
        printf("Result: ERROR - Incorrect sorting result.\n");

    free(a);
    free(expected);
}

/* 10. COMPARE ALL ALGORITHMS */
void compareAll(int n)
{
    if (n <= 0 || (size_t)n > (size_t)-1 / sizeof(int))
    {
        fprintf(stderr, "Invalid array size.\n");
        return;
    }

    int *a = malloc((size_t)n * sizeof(int));
    if (a == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        return;
    }

    generateArray(a, n);

    printf("\n======= COMPARISON =======\n");
    runOne(bubbleSort, "Bubble Sort", a, n);
    runOne(selectionSort, "Selection Sort", a, n);
    runOne(insertionSort, "Insertion Sort", a, n);
    runOne(mergeSort, "Merge Sort", a, n);
    runOne(quickSort, "Quick Sort", a, n);

    free(a);
}

/* 11. BIG ARRAY TEST */
void bigTest(void)
{
    int *a = malloc((size_t)BIG_SIZE * sizeof(int));
    int *smallArray = malloc((size_t)SMALL_SIZE * sizeof(int));

    if (a == NULL || smallArray == NULL)
    {
        free(a);
        free(smallArray);
        fprintf(stderr, "Memory allocation failed.\n");
        return;
    }

    generateArray(smallArray, SMALL_SIZE);
    generateArray(a, BIG_SIZE);

    printf("\n======= SMALL ARRAY TEST =======\n");
    runOne(bubbleSort, "Bubble Sort", smallArray, SMALL_SIZE);
    runOne(selectionSort, "Selection Sort", smallArray, SMALL_SIZE);
    runOne(insertionSort, "Insertion Sort", smallArray, SMALL_SIZE);

    printf("\n======= BIG ARRAY TEST =======\n");
    runOne(mergeSort, "Merge Sort", a, BIG_SIZE);
    runOne(quickSort, "Quick Sort", a, BIG_SIZE);

    free(smallArray);
    free(a);
}

/* 12. READ VALID INPUT */
int readNumber(const char message[])
{
    char line[100];
    char *end;
    long value;

    while (1)
    {
        printf("%s", message);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL)
            return -1;

        /* If the line did not fit, discard the rest and reject it. */
        if (strchr(line, '\n') == NULL && !feof(stdin))
        {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF)
                ;
            printf("Invalid input! Try again.\n");
            continue;
        }

        errno = 0;
        value = strtol(line, &end, 10);

        if (end != line && errno != ERANGE)
        {
            while (*end == ' ' || *end == '\t' ||
                   *end == '\n' || *end == '\r')
                end++;

            if (*end == '\0' && value >= 0 && value <= INT_MAX)
                return (int)value;
        }

        printf("Invalid input! Try again.\n");
    }
}

/* 13. MAIN MENU */
int main(void)
{
    int choice;
    int n;

    srand((unsigned int)time(NULL));

    while (1)
    {
        printf("\n");
        printf("======= SORTING ALGORITHMS =======\n");
        printf("1. Bubble Sort\n");
        printf("2. Selection Sort\n");
        printf("3. Insertion Sort\n");
        printf("4. Merge Sort\n");
        printf("5. Quick Sort\n");
        printf("6. Compare All Algorithms\n");
        printf("7. Big Array Test (100000)\n");
        printf("0. Exit\n");

        choice = readNumber("Your choice: ");

        if (choice == -1)
        {
            printf("\nInput closed. Goodbye!\n");
            break;
        }

        if (choice == 0)
        {
            printf("Goodbye!\n");
            break;
        }

        if (choice == 7)
        {
            bigTest();
            continue;
        }

        if (choice < 1 || choice > 6)
        {
            printf("Invalid choice!\n");
            continue;
        }

        if (choice == 6)
        {
            n = readNumber("Enter array size (1-100000): ");
            if (n == -1)
                break;

            if (n < 1 || n > BIG_SIZE)
            {
                printf("Invalid size!\n");
                continue;
            }

            if (n > 10000)
            {
                printf("Warning: Bubble, Selection and "
                       "Insertion may be very slow.\n");
            }

            compareAll(n);
            continue;
        }

        n = readNumber("Enter array size (1-100000): ");
        if (n == -1)
            break;

        if (n < 1 || n > BIG_SIZE)
        {
            printf("Invalid size!\n");
            continue;
        }

        if (n > 10000 && choice <= 3)
            printf("Warning: This algorithm may be very slow.\n");

        int *a = malloc((size_t)n * sizeof(int));
        if (a == NULL)
        {
            fprintf(stderr, "Memory allocation failed.\n");
            continue;
        }

        generateArray(a, n);

        switch (choice)
        {
            case 1:
                runOne(bubbleSort, "Bubble Sort", a, n);
                break;
            case 2:
                runOne(selectionSort, "Selection Sort", a, n);
                break;
            case 3:
                runOne(insertionSort, "Insertion Sort", a, n);
                break;
            case 4:
                runOne(mergeSort, "Merge Sort", a, n);
                break;
            case 5:
                runOne(quickSort, "Quick Sort", a, n);
                break;
            default:
                fprintf(stderr, "Unexpected menu choice.\n");
                break;
        }

        free(a);
    }

    return 0;
}
