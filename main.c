#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ai.h"
#include "backtracking.h"
#include "dp.h"
#include "greedy.h"
#include "sort.h"

#define CSV_FIELD_COUNT 8
#define CSV_MAX_FIELD_LEN 256
#define CSV_MAX_LINE_LEN 2048
#define DEFAULT_CSV_FILE "AlgorithmData.csv"
#define PATH_MAX_LEN 1024

/* Safe string copy used throughout the console output and CSV loader. */
static void copy_text(char *dst, size_t dstSize, const char *src) {
  if (dst == NULL || dstSize == 0)
    return;

  if (src == NULL) {
    dst[0] = '\0';
    return;
  }
  snprintf(dst, dstSize, "%s", src);
}

/* Remove BOM, tabs, spaces and line endings around a field. */
static void trim_text(char *text) {
  if (text == NULL || text[0] == '\0')
    return;

  unsigned char *bytes = (unsigned char *)text;
  if (bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) // UTF-8 BOM
    memmove(text, text + 3, strlen(text + 3) + 1);

  size_t start = 0;
  size_t len = strlen(text);
  while (start < len && isspace((unsigned char)text[start]))
    start++;

  size_t end = len;
  while (end > start && isspace((unsigned char)text[end - 1]))
    end--;

  if (start > 0)
    memmove(text, text + start, end - start);

  text[end - start] = '\0';
}

static char lower_ascii(char ch) { return (char)tolower((unsigned char)ch); }

static bool text_equals_ci(const char *left, const char *right) {
  if (left == NULL || right == NULL)
    return false;

  while (*left != '\0' && *right != '\0') {
    if (lower_ascii(*left) != lower_ascii(*right))
      return false;

    left++;
    right++;
  }
  return *left == '\0' && *right == '\0';
}

static bool text_contains_ci(const char *text, const char *needle) {
  if (text == NULL || needle == NULL || needle[0] == '\0')
    return false;

  size_t textLen = strlen(text);
  size_t needleLen = strlen(needle);
  if (needleLen > textLen)
    return false;

  for (size_t i = 0; i + needleLen <= textLen; i++) {
    size_t j = 0;
    while (j < needleLen && lower_ascii(text[i + j]) == lower_ascii(needle[j]))
      j++;

    if (j == needleLen)
      return true;
  }
  return false;
}

/* Parse one CSV row, including quoted fields and escaped quotes. */
static int parse_csv_line(const char *line, char fields[][CSV_MAX_FIELD_LEN],
                          int maxFields) {
  int fieldIndex = 0;
  int charIndex = 0;
  bool inQuotes = false;

  for (int i = 0; i < maxFields; i++)
    fields[i][0] = '\0';

  for (int i = 0; line[i] != '\0' && fieldIndex < maxFields; i++) {
    char ch = line[i];

    if (ch == '"') {
      if (inQuotes && line[i + 1] == '"') {
        if (charIndex < CSV_MAX_FIELD_LEN - 1)
          fields[fieldIndex][charIndex++] = '"';

        i++;
      } else
        inQuotes = !inQuotes;

    } else if (ch == ',' && !inQuotes) {
      fields[fieldIndex][charIndex] = '\0';
      trim_text(fields[fieldIndex]);
      fieldIndex++;
      charIndex = 0;
    } else if (ch != '\n' && ch != '\r') {
      if (charIndex < CSV_MAX_FIELD_LEN - 1)
        fields[fieldIndex][charIndex++] = ch;
    }
  }

  if (fieldIndex < maxFields) {
    fields[fieldIndex][charIndex] = '\0';
    trim_text(fields[fieldIndex]);
    fieldIndex++;
  }

  return fieldIndex;
}

static int parse_int_field(const char *text) {
  if (text == NULL || text[0] == '\0')
    return 0;

  return (int)strtol(text, NULL, 10);
}

/* Preset time budgets for the three required project scenarios. */
static int infer_available_time_from_name(const char *scenarioName) {
  if (text_contains_ci(scenarioName, "low-pressure") ||
      text_contains_ci(scenarioName, "low pressure"))
    return 25;
  if (text_contains_ci(scenarioName, "high-pressure") ||
      text_contains_ci(scenarioName, "high pressure"))
    return 12;
  if (text_contains_ci(scenarioName, "deadline"))
    return 20;
  return 0;
}

/* Sum all task durations in one scenario. */
static int scenario_total_required_time(const Scenario *scenario) {
  int total = 0;
  for (int i = 0; i < scenario->taskCount; i++)
    total += scenario->tasks[i].studyTime;

  return total;
}

/* Fill missing time budgets with 60% of total required time. */
static void finalize_scenarios(Scenario scenarios[], int scenarioCount) {
  for (int i = 0; i < scenarioCount; i++) {
    if (scenarios[i].availableTime <= 0) {
      int total = scenario_total_required_time(&scenarios[i]);
      scenarios[i].availableTime = (total * 60) / 100;
      if (scenarios[i].availableTime <= 0)
        scenarios[i].availableTime = 1;
    }
  }
}

/* Load scenario names and tasks from the CSV dataset. */
static bool load_scenarios_from_csv(const char *csvPath, Scenario scenarios[],
                                    int *outScenarioCount, char *error,
                                    size_t errorSize) {
  FILE *file = fopen(csvPath, "r");
  if (file == NULL) {
    snprintf(error, errorSize, "Unable to open CSV file: %s", csvPath);
    return false;
  }

  char line[CSV_MAX_LINE_LEN];
  char fields[CSV_FIELD_COUNT][CSV_MAX_FIELD_LEN];
  int scenarioCount = 0;
  int currentScenario = -1;
  int lineNumber = 0;

  while (fgets(line, sizeof(line), file) != NULL) {
    lineNumber++;
    parse_csv_line(line, fields, CSV_FIELD_COUNT);

    if (lineNumber == 1 || text_equals_ci(fields[1], "Task ID"))
      continue;

    /* A non-empty first column starts a new scenario. */
    if (fields[0][0] != '\0') {
      if (scenarioCount >= SS_MAX_SCENARIOS)
        break;

      currentScenario = scenarioCount;
      memset(&scenarios[currentScenario], 0, sizeof(Scenario));
      copy_text(scenarios[currentScenario].name,
                sizeof(scenarios[currentScenario].name), fields[0]);
      scenarios[currentScenario].availableTime =
          infer_available_time_from_name(fields[0]);
      scenarioCount++;
    }

    if (currentScenario < 0 || fields[1][0] == '\0' || fields[2][0] == '\0')
      continue;

    Scenario *scenario = &scenarios[currentScenario];
    if (scenario->taskCount >= SS_MAX_TASKS)
      continue;

    /* Remaining columns become one Task record. */
    Task *task = &scenario->tasks[scenario->taskCount++];
    task->id = parse_int_field(fields[1]);
    copy_text(task->name, sizeof(task->name), fields[2]);
    task->studyTime = parse_int_field(fields[3]);
    task->importance = parse_int_field(fields[4]);
    task->deadline = parse_int_field(fields[5]);
    task->difficulty = parse_int_field(fields[6]);
    copy_text(task->taskType, sizeof(task->taskType), fields[7]);
  }

  fclose(file);
  finalize_scenarios(scenarios, scenarioCount);

  if (scenarioCount == 0) {
    snprintf(error, errorSize, "No scenario rows were loaded from: %s",
             csvPath);
    return false;
  }

  *outScenarioCount = scenarioCount;
  return true;
}

/* Draw a simple ASCII table border. */
static void print_rule(const int widths[], int count) {
  putchar('+');
  for (int i = 0; i < count; i++) {
    for (int j = 0; j < widths[i] + 2; j++)
      putchar('-');

    putchar('+');
  }
  putchar('\n');
}

/* Print one row and shorten cells that are too long for the table. */
static void print_row(const char *cols[], const int widths[], int count) {
  putchar('|');
  for (int i = 0; i < count; i++) {
    char cell[256];
    copy_text(cell, sizeof(cell), cols[i]);

    if ((int)strlen(cell) > widths[i] && widths[i] >= 4) {
      cell[widths[i] - 3] = '.';
      cell[widths[i] - 2] = '.';
      cell[widths[i] - 1] = '.';
      cell[widths[i]] = '\0';
    }
    printf(" %-*s |", widths[i], cell);
  }

  putchar('\n');
}

/* Convert selected tasks into a compact ID list, for example "T1 T3". */
static void format_selected_ids(const AlgorithmResult *result, char *buffer,
                                size_t bufferSize) {
  buffer[0] = '\0';
  if (result->selectedCount == 0) {
    copy_text(buffer, bufferSize, "-");
    return;
  }

  for (int i = 0; i < result->selectedCount; i++) {
    char item[16];
    snprintf(item, sizeof(item), "%sT%d", i == 0 ? "" : " ",
             result->selectedTasks[i].id);
    if (strlen(buffer) + strlen(item) + 1 >= bufferSize) {
      strncat(buffer, "...", bufferSize - strlen(buffer) - 1);
      return;
    }
    strncat(buffer, item, bufferSize - strlen(buffer) - 1);
  }
}

/* Print the scenario overview used by menu options. */
static void print_scenario_list(const Scenario scenarios[], int scenarioCount) {
  const int widths[] = {4, 42, 7, 10, 10};
  const char *header[] = {"No.", "Scenario", "Tasks", "Required", "Available"};

  print_rule(widths, 5);
  print_row(header, widths, 5);
  print_rule(widths, 5);

  for (int i = 0; i < scenarioCount; i++) {
    char indexText[16];
    char taskText[16];
    char requiredText[16];
    char availableText[16];
    snprintf(indexText, sizeof(indexText), "%d", i + 1);
    snprintf(taskText, sizeof(taskText), "%d", scenarios[i].taskCount);
    snprintf(requiredText, sizeof(requiredText), "%d",
             scenario_total_required_time(&scenarios[i]));
    snprintf(availableText, sizeof(availableText), "%d",
             scenarios[i].availableTime);

    const char *row[] = {indexText, scenarios[i].name, taskText, requiredText,
                         availableText};
    print_row(row, widths, 5);
  }
  print_rule(widths, 5);
}

/* Print all task attributes for a single scenario. */
static void print_task_table(const Scenario *scenario) {
  const int widths[] = {4, 42, 5, 5, 8, 6, 11, 6};
  const char *header[] = {"ID",       "Task", "Time", "Imp",
                          "Deadline", "Diff", "Type", "Ratio"};

  print_rule(widths, 8);
  print_row(header, widths, 8);
  print_rule(widths, 8);

  for (int i = 0; i < scenario->taskCount; i++) {
    const Task *task = &scenario->tasks[i];
    char idText[16];
    char timeText[16];
    char importanceText[16];
    char deadlineText[16];
    char difficultyText[16];
    char ratioText[16];

    snprintf(idText, sizeof(idText), "T%d", task->id);
    snprintf(timeText, sizeof(timeText), "%d", task->studyTime);
    snprintf(importanceText, sizeof(importanceText), "%d", task->importance);
    snprintf(deadlineText, sizeof(deadlineText), "%d", task->deadline);
    snprintf(difficultyText, sizeof(difficultyText), "%d", task->difficulty);
    snprintf(ratioText, sizeof(ratioText), "%.2f", ss_task_ratio(task));

    const char *row[] = {idText,         task->name,   timeText,
                         importanceText, deadlineText, difficultyText,
                         task->taskType, ratioText};
    print_row(row, widths, 8);
  }
  print_rule(widths, 8);
}

/* Menu option 1: show the summary plus every scenario's task table. */
static void print_all_scenario_details(const Scenario scenarios[],
                                       int scenarioCount) {
  printf("\nScenario Details\n");
  for (int i = 0; i < scenarioCount; i++) {
    printf("\n%s\n", scenarios[i].name);
    printf("Available study time: %d hours\n", scenarios[i].availableTime);
    printf("Total required time:  %d hours\n",
           scenario_total_required_time(&scenarios[i]));
    print_task_table(&scenarios[i]);
  }
}

/* Dispatch to the selected algorithm without timing wrapper logic. */
static void execute_strategy_plain(const Scenario *scenario,
                                   StrategyType strategy,
                                   AlgorithmResult *result) {
  switch (strategy) {
  case STRATEGY_SORTING:
    ss_sort_run(scenario, result);
    break;
  case STRATEGY_GREEDY:
    ss_greedy_run(scenario, result);
    break;
  case STRATEGY_DP:
    ss_dp_run(scenario, result);
    break;
  case STRATEGY_BACKTRACKING:
    ss_backtracking_run(scenario, result);
    break;
  default:
    ss_result_init(result, strategy);
    break;
  }
}

/* Run one strategy and record execution time in milliseconds. */
static AlgorithmResult run_strategy_timed(const Scenario *scenario,
                                          StrategyType strategy) {
  AlgorithmResult result;
  clock_t start = clock();

  if (strategy == STRATEGY_AI_RECOMMENDATION) {
    AlgorithmResult predictedResult;

    /* AI first predicts a strategy, then reuses that strategy's result. */
    ss_result_init(&result, STRATEGY_AI_RECOMMENDATION);
    result.features = ss_ai_extract_features(scenario);
    result.recommendedStrategy = ss_ai_predict_strategy(&result.features);
    execute_strategy_plain(scenario, result.recommendedStrategy,
                           &predictedResult);

    result.orderedCount = predictedResult.orderedCount;
    result.selectedCount = predictedResult.selectedCount;
    result.totalStudyTime = predictedResult.totalStudyTime;
    result.totalImportance = predictedResult.totalImportance;
    for (int i = 0; i < predictedResult.orderedCount; i++)
      result.orderedTasks[i] = predictedResult.orderedTasks[i];
    for (int i = 0; i < predictedResult.selectedCount; i++)
      result.selectedTasks[i] = predictedResult.selectedTasks[i];
  } else
    execute_strategy_plain(scenario, strategy, &result);

  clock_t end = clock();
  result.elapsedMs = ((double)(end - start) * 1000.0) / CLOCKS_PER_SEC;
  return result;
}

/* Choose the best result by importance, then by lower time if tied. */
static StrategyType choose_best_strategy(const AlgorithmResult results[],
                                         int count) {
  int best = 0;
  for (int i = 1; i < count; i++) {
    if (results[i].totalImportance > results[best].totalImportance)
      best = i;
    else if (results[i].totalImportance == results[best].totalImportance &&
             results[i].totalStudyTime < results[best].totalStudyTime)
      best = i;
  }
  return results[best].strategy;
}

/* Short names keep AI comments readable inside the table. */
static const char *short_strategy_name(StrategyType strategy) {
  switch (strategy) {
  case STRATEGY_SORTING:
    return "Sorting";
  case STRATEGY_GREEDY:
    return "Greedy";
  case STRATEGY_DP:
    return "DP";
  case STRATEGY_BACKTRACKING:
    return "Backtracking";
  case STRATEGY_AI_RECOMMENDATION:
    return "AI";
  default:
    return "Unknown";
  }
}

/* Generate a one-line explanation for each algorithm result. */
static void comment_for_result(const AlgorithmResult *result,
                               StrategyType actualBest, char *buffer,
                               size_t bufferSize) {
  switch (result->strategy) {
  case STRATEGY_SORTING:
    copy_text(buffer, bufferSize, "Ranked prefix by importance/time ratio.");
    break;
  case STRATEGY_GREEDY:
    copy_text(buffer, bufferSize, "Selects best feasible ratio each step.");
    break;
  case STRATEGY_DP:
    copy_text(buffer, bufferSize, "Optimal 0/1 knapsack selection.");
    break;
  case STRATEGY_BACKTRACKING:
    copy_text(buffer, bufferSize, "Exhaustive validation for small cases.");
    break;
  case STRATEGY_AI_RECOMMENDATION:
    snprintf(buffer, bufferSize, "Predict %s; best match: %s.",
             short_strategy_name(result->recommendedStrategy),
             result->recommendedStrategy == actualBest ? "yes" : "no");
    break;
  default:
    copy_text(buffer, bufferSize, "-");
    break;
  }
}

/* Print the main performance comparison table. */
static void print_performance_table(const AlgorithmResult results[],
                                    int resultCount, StrategyType actualBest) {
  const int widths[] = {24, 27, 10, 10, 10, 44};
  const char *header[] = {"Strategy",   "Selected Tasks", "Time Used",
                          "Importance", "Exec ms",        "Comment"};

  print_rule(widths, 6);
  print_row(header, widths, 6);
  print_rule(widths, 6);

  for (int i = 0; i < resultCount; i++) {
    char selectedText[128];
    char timeText[32];
    char importanceText[32];
    char elapsedText[32];
    char commentText[160];

    format_selected_ids(&results[i], selectedText, sizeof(selectedText));
    snprintf(timeText, sizeof(timeText), "%d", results[i].totalStudyTime);
    snprintf(importanceText, sizeof(importanceText), "%d",
             results[i].totalImportance);
    snprintf(elapsedText, sizeof(elapsedText), "%.3f", results[i].elapsedMs);
    comment_for_result(&results[i], actualBest, commentText,
                       sizeof(commentText));

    const char *row[] = {ss_strategy_name(results[i].strategy),
                         selectedText,
                         timeText,
                         importanceText,
                         elapsedText,
                         commentText};
    print_row(row, widths, 6);
  }
  print_rule(widths, 6);
}

/* Run all strategies for one scenario and print comparison results. */
static void run_comparison_for_scenario(const Scenario *scenario) {
  printf("\nScenario: %s\n", scenario->name);
  printf("Available study time: %d hours\n", scenario->availableTime);
  printf("Total required time:  %d hours\n\n",
         scenario_total_required_time(scenario));

  print_task_table(scenario);
  putchar('\n');

  AlgorithmResult requiredResults[3];
  requiredResults[0] = run_strategy_timed(scenario, STRATEGY_SORTING);
  requiredResults[1] = run_strategy_timed(scenario, STRATEGY_GREEDY);
  requiredResults[2] = run_strategy_timed(scenario, STRATEGY_DP);

  /* "Actual best" is based only on the three required strategies. */
  StrategyType actualBest = choose_best_strategy(requiredResults, 3);
  AlgorithmResult aiResult =
      run_strategy_timed(scenario, STRATEGY_AI_RECOMMENDATION);

  AlgorithmResult allResults[5];
  allResults[0] = requiredResults[0];
  allResults[1] = requiredResults[1];
  allResults[2] = requiredResults[2];
  allResults[3] = aiResult;
  allResults[4] = run_strategy_timed(scenario, STRATEGY_BACKTRACKING);

  printf("Extracted AI Features\n");
  printf("Tasks=%d, TotalTime=%d, AvailableTime=%d, Pressure=%d%%, "
         "AvgImportance=%d, DeadlineTightness=%d%%, ImportanceVariation=%d\n\n",
         aiResult.features.numTasks, aiResult.features.totalRequiredTime,
         aiResult.features.availableTime, aiResult.features.timePressureRatio,
         aiResult.features.averageImportance,
         aiResult.features.deadlineTightness,
         aiResult.features.importanceVariation);
  print_performance_table(allResults, 5, actualBest);

  printf("Actual best required strategy: %s\n", ss_strategy_name(actualBest));
  printf("AI predicted strategy:        %s\n\n",
         ss_strategy_name(aiResult.recommendedStrategy));
}

/* Read one integer from the user; blank input returns -1. */
static int read_int_line(const char *prompt) {
  char line[64];
  printf("%s", prompt);
  fflush(stdout);
  if (fgets(line, sizeof(line), stdin) == NULL) {
    return 0;
  }
  trim_text(line);
  if (line[0] == '\0')
    return -1;

  return (int)strtol(line, NULL, 10);
}

/* Ask the user to choose a scenario index from the overview table. */
static int choose_scenario_index(const Scenario scenarios[],
                                 int scenarioCount) {
  print_scenario_list(scenarios, scenarioCount);
  int choice = read_int_line("Select scenario number: ");
  if (choice < 1 || choice > scenarioCount) {
    printf("Invalid scenario number.\n");
    return -1;
  }
  return choice - 1;
}

/* Let the user change the available study time of an existing scenario. */
static void adjust_available_time(Scenario scenarios[], int scenarioCount) {
  int index = choose_scenario_index(scenarios, scenarioCount);
  if (index < 0)
    return;

  int value = read_int_line("New available study time: ");
  if (value <= 0) {
    printf("Available time must be positive.\n");
    return;
  }

  scenarios[index].availableTime = value;
  printf("Updated available time for %s to %d hours.\n", scenarios[index].name,
         value);
}

static void plan_study(void);

/* Display the rule-based AI decision tree. */
static void print_ai_rules(void) {
  printf("\nManual Decision Tree Rules (2 features, 2 levels)\n");
  printf("1. If time pressure < 150%%, "
         "predict Sorting-based ranking.\n");
  printf("2. Else if deadline tightness >= 75%%, "
         "predict Greedy strategy.\n");
  printf("3. Otherwise, predict Dynamic Programming "
         "(optimal selection under time limit).\n\n");
}

/* Main command menu. */
static void print_menu(void) {
  printf("\nMain Menu\n");
  printf("1. List scenarios\n");
  printf("2. Run one scenario comparison\n");
  printf("3. Run all scenario comparisons\n");
  printf("4. Adjust scenario available time\n");
  printf("5. Show AI decision tree rules\n");
  printf("6. Plan your Study Time!\n");
  printf("0. Exit\n");
}

/* Handle menu choices until the user exits. */
static void menu_loop(Scenario scenarios[], int scenarioCount) {
  while (true) {
    print_menu();
    int choice = read_int_line("Choose: ");
    switch (choice) {
    case 1:
      print_scenario_list(scenarios, scenarioCount);
      print_all_scenario_details(scenarios, scenarioCount);
      break;
    case 2: {
      int index = choose_scenario_index(scenarios, scenarioCount);
      if (index >= 0)
        run_comparison_for_scenario(&scenarios[index]);

    } break;
    case 3:
      for (int i = 0; i < scenarioCount; i++)
        run_comparison_for_scenario(&scenarios[i]);
      break;
    case 4:
      adjust_available_time(scenarios, scenarioCount);
      break;
    case 5:
      print_ai_rules();
      break;
    case 6:
      plan_study();
      break;
    case 0:
      return;
    default:
      printf("Invalid option.\n");
      break;
    }
  }
}


/* Read one text field and trim surrounding whitespace. */
static void read_text_line(const char *prompt, char *buffer, size_t bufferSize) {
  printf("%s", prompt);
  fflush(stdout);
  if (fgets(buffer, (int)bufferSize, stdin) == NULL) {
    buffer[0] = '\0';
    return;
  }
  trim_text(buffer);
}


/* Build a custom scenario from user input and compare all strategies. */
static void plan_study(void) {
  printf("     Plan Your Study Time!\n");

  int taskCount = read_int_line("How many of study tasks? ");
  if (taskCount <= 0) {
    printf("Invalid number of tasks. Returning to menu.\n");
    return;
  }
  if (taskCount > SS_MAX_TASKS) {
    printf("Too many tasks (max %d). Capping to %d.\n", SS_MAX_TASKS,
           SS_MAX_TASKS);
    taskCount = SS_MAX_TASKS;
  }

  Scenario scenario;
  memset(&scenario, 0, sizeof(Scenario));
  copy_text(scenario.name, sizeof(scenario.name), "User Planned Study");
  scenario.taskCount = taskCount;

  for (int i = 0; i < taskCount; i++) {
    Task *task = &scenario.tasks[i];
    char prompt[128];

    task->id = i + 1;


    snprintf(prompt, sizeof(prompt), "T%d Name: ", i + 1);
    read_text_line(prompt, task->name, sizeof(task->name));
    if (task->name[0] == '\0') {
      snprintf(task->name, sizeof(task->name), "Task %d", i + 1);
    }

    snprintf(prompt, sizeof(prompt), "T%d Estimated Study Time (hours): ",
             i + 1);
    task->studyTime = read_int_line(prompt);
    if (task->studyTime < 0) task->studyTime = 0;

    snprintf(prompt, sizeof(prompt), "T%d Importance (1-10): ", i + 1);
    task->importance = read_int_line(prompt);
    if (task->importance < 0) task->importance = 0;

    snprintf(prompt, sizeof(prompt), "T%d Deadline (days): ", i + 1);
    task->deadline = read_int_line(prompt);
    if (task->deadline < 0) task->deadline = 0;

    snprintf(prompt, sizeof(prompt), "T%d Difficulty (1-5): ", i + 1);
    task->difficulty = read_int_line(prompt);
    if (task->difficulty < 0) task->difficulty = 0;

    snprintf(prompt, sizeof(prompt),
             "T%d Task Type (Revision/Assignment/Tutorial/Practice): ", i + 1);
    read_text_line(prompt, task->taskType, sizeof(task->taskType));
    if (task->taskType[0] == '\0') {
      copy_text(task->taskType, sizeof(task->taskType), "General");
    }

    putchar('\n');
  }

  int totalRequired = scenario_total_required_time(&scenario);
  printf("Total required study time: %d hours\n", totalRequired);
  int availableTime = read_int_line("Your available study time (hours): ");
  if (availableTime <= 0) {
    /* Default capacity keeps custom scenarios runnable even with blank input. */
    availableTime = (totalRequired * 60) / 100;
    if (availableTime <= 0) availableTime = 1;
    printf("Using default available time: %d hours (60%% of total)\n",
           availableTime);
  }
  scenario.availableTime = availableTime;

  run_comparison_for_scenario(&scenario);
}

int main(int argc, char *argv[]) {
  (void)argc;
  char csvPath[PATH_MAX_LEN];

  /* Load the CSV from the same directory as the executable. */
  const char *slash = strrchr(argv[0], '/');
  const char *backslash = strrchr(argv[0], '\\');
  if (backslash != NULL && (slash == NULL || backslash > slash))
    slash = backslash;

  if (slash == NULL) {
    snprintf(csvPath, sizeof(csvPath), "%s", DEFAULT_CSV_FILE);
  } else {
    size_t dirLen = (size_t)(slash - argv[0] + 1);
    snprintf(csvPath, sizeof(csvPath), "%.*s%s", (int)dirLen, argv[0],
             DEFAULT_CSV_FILE);
  }

  Scenario scenarios[SS_MAX_SCENARIOS];
  int scenarioCount = 0;
  char error[256];
  if (!load_scenarios_from_csv(csvPath, scenarios, &scenarioCount, error,
                               sizeof(error))) {
    printf("%s\n", error);
    return EXIT_FAILURE;
  }

  printf("\nStudySmart AI\n");
  printf("Sorting, greedy, DP, backtracking and AI recommendation.\n\n");
  printf("Loaded %d scenario from %s.\n", scenarioCount, csvPath);
  menu_loop(scenarios, scenarioCount);
  return EXIT_SUCCESS;
}
