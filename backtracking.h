#ifndef STUDYSMART_BACKTRACKING_H
#define STUDYSMART_BACKTRACKING_H

#include "definition.h"

/* State used while exploring all task choices. */
typedef struct BacktrackingContext {
  const Scenario *scenario;
  bool current[SS_MAX_TASKS];
  bool best[SS_MAX_TASKS];
  int bestImportance;
  int bestTime;
} BacktrackingContext;

/* Keep the subset with higher importance; tie-break by lower study time. */
static void ss_bt_save_best(BacktrackingContext *ctx, int currentImportance,
                            int currentTime) {
  if (currentImportance > ctx->bestImportance ||
      (currentImportance == ctx->bestImportance &&
       currentTime < ctx->bestTime)) {
    ctx->bestImportance = currentImportance;
    ctx->bestTime = currentTime;
    for (int i = 0; i < ctx->scenario->taskCount; i++)
      ctx->best[i] = ctx->current[i];
  }
}

/* Try both choices for each task: skip it or take it if time allows. 
 All tasks have been decided, so evaluate this subset. */
static void ss_bt_search(BacktrackingContext *ctx, int index, int currentTime,
                         int currentImportance) {
  if (index == ctx->scenario->taskCount) {
    ss_bt_save_best(ctx, currentImportance, currentTime);
    return;
  }

  ctx->current[index] = false;
  ss_bt_search(ctx, index + 1, currentTime, currentImportance);

  const Task *task = &ctx->scenario->tasks[index];
  if (currentTime + task->studyTime <= ctx->scenario->availableTime) {
    ctx->current[index] = true;
    ss_bt_search(ctx, index + 1, currentTime + task->studyTime,
                 currentImportance + task->importance);
    ctx->current[index] = false;
  }
}

/* Run exhaustive search and copy the best subset into AlgorithmResult. */
static void ss_backtracking_run(const Scenario *scenario,
                                AlgorithmResult *result) {
  ss_result_init(result, STRATEGY_BACKTRACKING);
  if (scenario == NULL || result == NULL)
    return;

  BacktrackingContext ctx;
  memset(&ctx, 0, sizeof(ctx));
  ctx.scenario = scenario;
  ctx.bestTime = scenario->availableTime + 1;

  /* Start with no selected tasks and zero total time/importance ratio. */
  ss_bt_search(&ctx, 0, 0, 0);

  for (int i = 0; i < scenario->taskCount; i++) {
    if (ctx.best[i]) {
      result->orderedTasks[result->orderedCount++] = scenario->tasks[i];
      ss_result_add_selected(result, &scenario->tasks[i]);
    }
  }
}

#endif
