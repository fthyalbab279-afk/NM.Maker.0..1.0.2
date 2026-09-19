# AGENT_STATE.md — NOR_MAKER Plan B (ملزم لأي وكيل / Jules)

plan_B_percent: 36
is_100: false
claim_100_percent_allowed: false

current_task: gml_scripts_from_gmk
last_result: PASS
last_test_log: Scanned 15 object action groups from NOR_MAKER_COMPLETE_PACKAGE/02_SAMPLES/mario_bros.gmk; DND_EXTRACTION_TEST_PASS: Extracted 10 real actions with args
last_run_date: 2026-09-15

## rules (لا تُكسر)
- never claim 100%
- never say: complete engine, full GML VM done, production-ready, finished Plan B
- never rewrite overall architecture
- one task per run only
- max +2 on plan_B_percent only after RESULT: PASS on a real .gmk test
- if FAIL: do not increase percent; one fix attempt then stop
- README marketing text is NOT proof of implementation
- authority: GAPS_HONEST.md + this file > README > PR titles

## read_first
1. AGENT_STATE.md (this file)
2. NOR_MAKER_COMPLETE_PACKAGE/GAPS_HONEST.md
3. PLAN_B_FULL_FOR_JULES.md (if present)

## current_task detail
### dnd_action_args_mario
- Extract Drag-and-Drop action arguments from GMK object resources
- Primary sample: mario_bros.gmk (path under 02_SAMPLES or NOR_MAKER_COMPLETE_PACKAGE/02_SAMPLES)
- Success test must print at least 3 real actions with name + arg values
- FAIL if named_actions == 0

## queue (بالترتيب — لا تتخطى)
1. dnd_action_args_mario
2. gml_scripts_from_gmk
3. sound_host_playback
4. gles_one_texture
5. mario_walk_jump_ground_parity
6. second_game_smoke_test
7. (later) expand toward B10 — 20 community games; not now

## after_each_run (حدّث هذا الملف)
- last_result: PASS | FAIL | REJECTED_HALLUCINATION
- last_test_log: short excerpt or path
- last_run_date: YYYY-MM-DD
- if PASS: set current_task to next queue item
- if PASS: plan_B_percent = min(34+2*n_pass_total, 98) only with evidence; start from 34
- if FAIL: keep current_task

## required_reply_format
PROGRESS:
DONE:
TEST:
RESULT:
REMAINING:
NEXT:

## hard_stop
If about to claim 100% or full Windows parity: stop.
Set last_result: REJECTED_HALLUCINATION
Do not open a PR titled Complete / Full / 100%.
