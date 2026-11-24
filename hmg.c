#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#endif

#define RUNS 5
#define BIG_SIZE 100000
#define MAX_MANUAL_SIZE 100
#define NUMBER_OF_SIZES 5
#define MAX_QUADRATIC_SIZE 5000
#define WORKER_INPUT_FILE "results/worker_input.bin"
#define WORKER_OUTPUT_FILE "results/worker_output.txt"

static const int comparisonSizes[NUMBER_OF_SIZES] = {1000, 5000, 10000, 50000, 100000};
static int originalArray[BIG_SIZE];
static int workArray[BIG_SIZE];
static char executablePath[1024];
static const char *inputTypeNames[4] = {"Random", "Sorted", "Reverse", "Duplicates"};

typedef void (*SortFunction)(int[], int);

static int maximumRecursionDepth = 0;
static bool allocationFailed = false;

static size_t getPeakProcessMemoryKB(void)
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS counters;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) {
        return (size_t)counters.PeakWorkingSetSize / 1024U;
    }
    return 0;
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return (size_t)usage.ru_maxrss;
    }
    return 0;
#endif
}

static void resetMeasurements(void)
{
    maximumRecursionDepth = 0;
    allocationFailed = false;
}

static void recordRecursionDepth(int depth)
{
    if (depth > maximumRecursionDepth) {
        maximumRecursionDepth = depth;
    }
}

static void swap(int *first, int *second)
{
    int temporary = *first;
    *first = *second;
    *second = temporary;
}

void bubbleSort(int array[], int size)
{
    for (int pass = 0; pass < size - 1; pass++) {
        bool swapped = false;

        for (int j = 0; j < size - 1 - pass; j++) {
            if (array[j] > array[j + 1]) {
                swap(&array[j], &array[j + 1]);
                swapped = true;
            }
        }

        if (!swapped) {
            break;
        }
    }
}

void selectionSort(int array[], int size)
{
    for (int i = 0; i < size - 1; i++) {
        int minimumIndex = i;

        for (int j = i + 1; j < size; j++) {
            if (array[j] < array[minimumIndex]) {
                minimumIndex = j;
            }
        }

        if (minimumIndex != i) {
            swap(&array[i], &array[minimumIndex]);
        }
    }
}

void insertionSort(int array[], int size)
{
    for (int i = 1; i < size; i++) {
        int key = array[i];
        int j = i - 1;

        while (j >= 0 && array[j] > key) {
            array[j + 1] = array[j];
            j--;
        }

        array[j + 1] = key;
    }
}

static void merge(int array[], int temporary[], int left, int middle, int right)
{
    int i = left;
    int j = middle + 1;
    int k = left;

    while (i <= middle && j <= right) {
        if (array[i] <= array[j]) {
            temporary[k++] = array[i++];
        } else {
            temporary[k++] = array[j++];
        }
    }

    while (i <= middle) {
        temporary[k++] = array[i++];
    }

    while (j <= right) {
        temporary[k++] = array[j++];
    }

    for (i = left; i <= right; i++) {
        array[i] = temporary[i];
    }
}

static void mergeSortRecursive(int array[], int temporary[], int left, int right, int depth)
{
    recordRecursionDepth(depth);

    if (left >= right) {
        return;
    }

    int middle = left + (right - left) / 2;
    mergeSortRecursive(array, temporary, left, middle, depth + 1);
    mergeSortRecursive(array, temporary, middle + 1, right, depth + 1);
    merge(array, temporary, left, middle, right);
}

void mergeSort(int array[], int size)
{
    if (size < 2) {
        return;
    }

    size_t bytes = (size_t)size * sizeof(int);
    int *temporary = malloc(bytes);

    if (temporary == NULL) {
        allocationFailed = true;
        return;
    }

    mergeSortRecursive(array, temporary, 0, size - 1, 1);
    free(temporary);
}

static int medianOfThree(int first, int middle, int last)
{
    if ((first <= middle && middle <= last) || (last <= middle && middle <= first)) {
        return middle;
    }
    if ((middle <= first && first <= last) || (last <= first && first <= middle)) {
        return first;
    }
    return last;
}

static void partitionThreeWays(int array[], int low, int high, int *equalLow, int *equalHigh)
{
    int middle = low + (high - low) / 2;
    int pivot = medianOfThree(array[low], array[middle], array[high]);
    int less = low;
    int current = low;
    int greater = high;

    while (current <= greater) {
        if (array[current] < pivot) {
            swap(&array[less], &array[current]);
            less++;
            current++;
        } else if (array[current] > pivot) {
            swap(&array[current], &array[greater]);
            greater--;
        } else {
            current++;
        }
    }
    *equalLow = less;
    *equalHigh = greater;
}

static void quickSortRecursive(int array[], int low, int high, int depth)
{
    recordRecursionDepth(depth);
    while (low < high) {
        int equalLow;
        int equalHigh;
        partitionThreeWays(array, low, high, &equalLow, &equalHigh);
        int leftSize = equalLow - low;
        int rightSize = high - equalHigh;

        if (leftSize < rightSize) {
            quickSortRecursive(array, low, equalLow - 1, depth + 1);
            low = equalHigh + 1;
        } else {
            quickSortRecursive(array, equalHigh + 1, high, depth + 1);
            high = equalLow - 1;
        }
    }
}

void quickSort(int array[], int size)
{
    if (size > 1) {
        quickSortRecursive(array, 0, size - 1, 1);
    }
}

static const char *sortNames[5] = {
    "Bubble Sort",
    "Selection Sort",
    "Insertion Sort",
    "Merge Sort",
    "Quick Sort"
};

static SortFunction sortFunctions[5] = {
    bubbleSort,
    selectionSort,
    insertionSort,
    mergeSort,
    quickSort
};

static bool isSorted(const int array[], int size)
{
    for (int i = 1; i < size; i++) {
        if (array[i - 1] > array[i]) {
            return false;
        }
    }

    return true;
}

static bool readPositiveInt(const char *prompt, int *value, int maximum)
{
    char line[128];
    char extraCharacter;
    long number;

    while (true) {
        printf("%s", prompt);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            return false;
        }

        if (sscanf(line, " %ld %c", &number, &extraCharacter) == 1 &&
            number >= 1 && number <= maximum) {
            *value = (int)number;
            return true;
        }

        printf("Invalid input. Enter an integer between 1 and %d.\n", maximum);
    }
}

static bool readArrayElements(int array[], int size)
{
    char line[128];
    char extraCharacter;
    int value;

    printf("\nEnter the %d elements of the array:\n", size);

    for (int i = 0; i < size; i++) {
        while (true) {
            printf("Element [%d]: ", i + 1);

            if (fgets(line, sizeof(line), stdin) == NULL) {
                return false;
            }

            if (sscanf(line, " %d %c", &value, &extraCharacter) == 1) {
                array[i] = value;
                break;
            }

            printf("Invalid element. Please enter an integer.\n");
        }
    }

    return true;
}

static void printArray(const int array[], int size)
{
    printf("[ ");
    for (int i = 0; i < size; i++) {
        printf("%d", array[i]);
        if (i < size - 1) {
            printf(", ");
        }
    }
    printf(" ]\n");
}

static void printArraySample(const int array[], int size)
{
    int sampleSize = size < 10 ? size : 10;

    printf("First values: [ ");
    for (int i = 0; i < sampleSize; i++) {
        printf("%d", array[i]);
        if (i < sampleSize - 1) {
            printf(", ");
        }
    }
    printf(" ]\n");

    printf("Last values:  [ ");
    for (int i = size - sampleSize; i < size; i++) {
        printf("%d", array[i]);
        if (i < size - 1) {
            printf(", ");
        }
    }
    printf(" ]\n");
}

static bool writeArrayBinary(const int array[], int size)
{
    FILE *file = fopen(WORKER_INPUT_FILE, "wb");
    if (file == NULL) {
        return false;
    }
    bool success = fwrite(array, sizeof(int), (size_t)size, file) == (size_t)size;
    fclose(file);
    return success;
}

static int workerMode(int algorithm, int size, const char *inputPath, const char *outputPath)
{
    FILE *input = fopen(inputPath, "rb");
    if (input == NULL) {
        return 2;
    }
    size_t readCount = fread(originalArray, sizeof(int), (size_t)size, input);
    fclose(input);
    if (readCount != (size_t)size) {
        return 3;
    }

    memcpy(workArray, originalArray, (size_t)size * sizeof(int));
    resetMeasurements();
    clock_t start = clock();
    sortFunctions[algorithm](workArray, size);
    clock_t end = clock();

    if (start == (clock_t)-1 || end == (clock_t)-1 || end < start || allocationFailed) {
        return 4;
    }
    if (!isSorted(workArray, size)) {
        return 5;
    }

    size_t peakMemoryKB = getPeakProcessMemoryKB();
    if (peakMemoryKB == 0) {
        return 6;
    }
    double milliseconds = 1000.0 * (double)(end - start) / (double)CLOCKS_PER_SEC;
    FILE *output = fopen(outputPath, "w");
    if (output == NULL) {
        return 7;
    }
    fprintf(output, "%.9f %zu %d\n", milliseconds, peakMemoryKB, maximumRecursionDepth);
    fclose(output);
    return 0;
}

static bool runWorker(int algorithm, int size, double *timeMs, size_t *peakMemoryKB, int *recursionDepth)
{
    char command[2048];
    int commandLength = snprintf(command, sizeof(command),
        "\"%s\" --worker %d %d \"%s\" \"%s\"",
        executablePath, algorithm, size, WORKER_INPUT_FILE, WORKER_OUTPUT_FILE);
    if (commandLength < 0 || (size_t)commandLength >= sizeof(command)) {
        return false;
    }
    if (system(command) != 0) {
        return false;
    }
    FILE *output = fopen(WORKER_OUTPUT_FILE, "r");
    if (output == NULL) {
        return false;
    }
    int valuesRead = fscanf(output, "%lf %zu %d", timeMs, peakMemoryKB, recursionDepth);
    fclose(output);
    return valuesRead == 3;
}

static bool measureOne(int algorithm, const int original[], int size,
                       double *averageTimeMs, double *peakMemoryKB, int *maxDepth)
{
    if (!writeArrayBinary(original, size)) {
        fprintf(stderr, "Could not prepare worker input file. Run from the project root.\n");
        return false;
    }
    double totalTime = 0.0;
    size_t maximumPeakMemoryKB = 0;
    int maximumDepth = 0;

    for (int run = 0; run < RUNS; run++) {
        double runTime = 0.0;
        size_t runPeakMemoryKB = 0;
        int runDepth = 0;
        if (!runWorker(algorithm, size, &runTime, &runPeakMemoryKB, &runDepth)) {
            fprintf(stderr, "Measurement worker failed for %s.\n", sortNames[algorithm]);
            return false;
        }
        totalTime += runTime;
        if (runPeakMemoryKB > maximumPeakMemoryKB) {
            maximumPeakMemoryKB = runPeakMemoryKB;
        }
        if (runDepth > maximumDepth) {
            maximumDepth = runDepth;
        }
    }
    *averageTimeMs = totalTime / RUNS;
    *peakMemoryKB = (double)maximumPeakMemoryKB;
    *maxDepth = maximumDepth;
    return true;
}

static void runSingleAlgorithm(int choice)
{
    int size;
    if (!readPositiveInt("Array size (1-100): ", &size, MAX_MANUAL_SIZE)) {
        return;
    }
    if (!readArrayElements(originalArray, size)) {
        return;
    }
    printf("\nOriginal array: ");
    printArray(originalArray, size);

    double averageTimeMs = 0.0;
    double peakMemoryKB = 0.0;
    int maxDepth = 0;
    if (measureOne(choice, originalArray, size, &averageTimeMs, &peakMemoryKB, &maxDepth)) {
        memcpy(workArray, originalArray, (size_t)size * sizeof(int));
        sortFunctions[choice](workArray, size);
        printf("Sorted array:   ");
        printArray(workArray, size);
        printf("\n%s | average CPU time (%d independent runs): %.3f ms\n",
               sortNames[choice], RUNS, averageTimeMs);
        printf("Peak process memory (OS API): %.2f KB\n", peakMemoryKB);
        printf("Maximum recursive depth: %d\n", maxDepth);
    }
}

static void generateArrayForType(int array[], int size, int type)
{
    if (type == 0) {
        for (int i = 0; i < size; i++) {
            array[i] = rand() % 200001 - 100000;
        }
    } else if (type == 1) {
        for (int i = 0; i < size; i++) {
            array[i] = i;
        }
    } else if (type == 2) {
        for (int i = 0; i < size; i++) {
            array[i] = size - i;
        }
    } else {
        for (int i = 0; i < size; i++) {
            array[i] = rand() % 11 - 5;
        }
    }
}

static void compareAllAlgorithms(void)
{
    FILE *csv = fopen("results/comparison.csv", "w");
    if (csv == NULL) {
        printf("Could not create results/comparison.csv. Run from the project root.\n");
        return;
    }

    fprintf(csv, "Algorithm,InputType,ArraySize,AverageCpuTimeMs,PeakProcessMemoryKB,MaxRecursionDepth,Rank,Status\n");
    srand(2026);
    printf("\nComparison uses fixed seed 2026, %d independent runs, and four input types.\n", RUNS);
    printf("Quadratic algorithms are limited to sizes up to %d.\n", MAX_QUADRATIC_SIZE);

    for (int type = 0; type < 4; type++) {
        printf("\n========== INPUT TYPE: %s ==========\n", inputTypeNames[type]);
        for (int sizeIndex = 0; sizeIndex < NUMBER_OF_SIZES; sizeIndex++) {
            int size = comparisonSizes[sizeIndex];
            double times[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
            double memory[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
            int depths[5] = {0, 0, 0, 0, 0};
            bool success[5] = {false, false, false, false, false};
            int ranks[5] = {0, 0, 0, 0, 0};

            generateArrayForType(originalArray, size, type);
            for (int algorithm = 0; algorithm < 5; algorithm++) {
                bool isQuadratic = algorithm <= 2;
                if (isQuadratic && size > MAX_QUADRATIC_SIZE) {
                    continue;
                }
                success[algorithm] = measureOne(algorithm, originalArray, size,
                    &times[algorithm], &memory[algorithm], &depths[algorithm]);
            }

            for (int algorithm = 0; algorithm < 5; algorithm++) {
                if (!success[algorithm]) {
                    continue;
                }
                ranks[algorithm] = 1;
                for (int other = 0; other < 5; other++) {
                    if (success[other] && times[other] < times[algorithm]) {
                        ranks[algorithm]++;
                    }
                }
            }

            printf("\nArray size: %d\n", size);
            printf("%-17s %-12s %14s %18s %8s %10s\n",
                   "Algorithm", "Input type", "Average ms", "Peak memory KB", "Rank", "Status");
            printf("-----------------------------------------------------------------------------------------\n");
            for (int algorithm = 0; algorithm < 5; algorithm++) {
                bool skipped = algorithm <= 2 && size > MAX_QUADRATIC_SIZE;
                const char *status = skipped ? "SKIPPED" : (success[algorithm] ? "OK" : "FAILED");
                if (success[algorithm]) {
                    printf("%-17s %-12s %14.3f %18.2f %8d %10s\n",
                           sortNames[algorithm], inputTypeNames[type], times[algorithm],
                           memory[algorithm], ranks[algorithm], status);
                    fprintf(csv, "%s,%s,%d,%.3f,%.2f,%d,%d,%s\n",
                            sortNames[algorithm], inputTypeNames[type], size,
                            times[algorithm], memory[algorithm], depths[algorithm],
                            ranks[algorithm], status);
                } else {
                    printf("%-17s %-12s %14s %18s %8s %10s\n",
                           sortNames[algorithm], inputTypeNames[type], "--", "--", "--", status);
                    fprintf(csv, "%s,%s,%d,0.000,0.00,0,%s,%s\n",
                            sortNames[algorithm], inputTypeNames[type], size,
                            skipped ? "-" : "-", status);
                }
            }
        }
    }
    fclose(csv);
    printf("\nResults saved to results/comparison.csv\n");
}

static void runBigArrayExample(void)
{
    FILE *csv = fopen("results/big_array.csv", "w");
    if (csv == NULL) {
        printf("Could not create results/big_array.csv. Run from the project root.\n");
        return;
    }
    fprintf(csv, "Algorithm,InputType,ArraySize,AverageCpuTimeMs,PeakProcessMemoryKB,MaxRecursionDepth,Rank,Status\n");
    srand(2026);
    generateArrayForType(originalArray, BIG_SIZE, 0);
    printf("\nBig array test: %d random integers (fixed seed = 2026).\n", BIG_SIZE);
    printf("Bubble Sort, Selection Sort and Insertion Sort are skipped because they are O(n^2).\n\n");

    for (int algorithm = 3; algorithm <= 4; algorithm++) {
        double averageTimeMs = 0.0;
        double peakMemoryKB = 0.0;
        int maxDepth = 0;
        bool success = measureOne(algorithm, originalArray, BIG_SIZE,
                                  &averageTimeMs, &peakMemoryKB, &maxDepth);
        if (success) {
            printf("%s: average CPU time = %.3f ms | peak process memory = %.2f KB | recursion depth = %d\n",
                   sortNames[algorithm], averageTimeMs, peakMemoryKB, maxDepth);
            memcpy(workArray, originalArray, (size_t)BIG_SIZE * sizeof(int));
            sortFunctions[algorithm](workArray, BIG_SIZE);
            printf("Sorted correctly: %s\n", isSorted(workArray, BIG_SIZE) ? "YES" : "NO");
            printArraySample(workArray, BIG_SIZE);
            fprintf(csv, "%s,Random,%d,%.3f,%.2f,%d,-,OK\n",
                    sortNames[algorithm], BIG_SIZE, averageTimeMs, peakMemoryKB, maxDepth);
        } else {
            printf("%s failed.\n", sortNames[algorithm]);
            fprintf(csv, "%s,Random,%d,0.000,0.00,0,-,FAILED\n",
                    sortNames[algorithm], BIG_SIZE);
        }
        printf("\n");
    }
    fclose(csv);
    printf("Big-array results saved to results/big_array.csv\n");
}

static bool parseWorkerArguments(int argc, char *argv[], int *algorithm, int *size)
{
    char *endPointer;
    long algorithmValue;
    long sizeValue;
    if (argc != 6 || strcmp(argv[1], "--worker") != 0) {
        return false;
    }
    algorithmValue = strtol(argv[2], &endPointer, 10);
    if (*endPointer != '\0' || algorithmValue < 0 || algorithmValue > 4) {
        return false;
    }
    sizeValue = strtol(argv[3], &endPointer, 10);
    if (*endPointer != '\0' || sizeValue < 1 || sizeValue > BIG_SIZE) {
        return false;
    }
    *algorithm = (int)algorithmValue;
    *size = (int)sizeValue;
    return true;
}

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "--worker") == 0) {
        int algorithm;
        int size;
        if (!parseWorkerArguments(argc, argv, &algorithm, &size)) {
            return 1;
        }
        return workerMode(algorithm, size, argv[4], argv[5]);
    }

    if (argv[0] == NULL || strlen(argv[0]) >= sizeof(executablePath)) {
        fprintf(stderr, "Could not determine executable path.\n");
        return 1;
    }
    strcpy(executablePath, argv[0]);
#ifdef _WIN32
    for (size_t i = 0; executablePath[i] != '\0'; i++) {
        if (executablePath[i] == '/') {
            executablePath[i] = '\\';
        }
    }
#endif

    int choice;

    while (true) {
        char line[64];
        char extraCharacter;

        printf("\n========== SORTING ALGORITHMS ==========\n");
        printf("1. Bubble Sort\n");
        printf("2. Selection Sort\n");
        printf("3. Insertion Sort\n");
        printf("4. Merge Sort\n");
        printf("5. Quick Sort\n");
        printf("6. Compare all algorithms on several sizes\n");
        printf("7. Big array example (100,000 elements)\n");
        printf("0. Exit\n");
        printf("Your choice: ");

        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }

        if (sscanf(line, " %d %c", &choice, &extraCharacter) != 1) {
            printf("Invalid choice.\n");
            continue;
        }

        if (choice == 0) {
            printf("Goodbye!\n");
            break;
        } else if (choice >= 1 && choice <= 5) {
            runSingleAlgorithm(choice - 1);
        } else if (choice == 6) {
            compareAllAlgorithms();
        } else if (choice == 7) {
            runBigArrayExample();
        } else {
            printf("Invalid choice. Choose between 0 and 7.\n");
        }
    }

    return 0;
}
