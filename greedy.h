#ifndef STUDYSMART_GREEDY_H
#define STUDYSMART_GREEDY_H

#include "definition.h"

static int ss_greedy_find_best_feasible(const Scenario *scenario,
                                        const bool used[], int usedTime) {
  int bestIndex = -1;

  for (int i = 0; i < scenario->taskCount; i++) {
    if (used[i]) {
      continue;
    }
    if (usedTime + scenario->tasks[i].studyTime > scenario->availableTime) {
      continue;
    }
    if (bestIndex < 0 ||
        ss_task_priority_cmp(&scenario->tasks[i], &scenario->tasks[bestIndex]) <
            0) {
      bestIndex = i;
    }
  }

  return bestIndex;
}

static void ss_greedy_run(const Scenario *scenario, AlgorithmResult *result) {
  ss_result_init(result, STRATEGY_GREEDY);
  if (scenario == NULL || result == NULL) {
    return;
  }

  bool used[SS_MAX_TASKS] = {false};

  while (true) {
    int bestIndex =
        ss_greedy_find_best_feasible(scenario, used, result->totalStudyTime);
    if (bestIndex < 0) {
      break;
    }

    used[bestIndex] = true;
    result->orderedTasks[result->orderedCount++] = scenario->tasks[bestIndex];
    ss_result_add_selected(result, &scenario->tasks[bestIndex]);
  }
}

#endif
