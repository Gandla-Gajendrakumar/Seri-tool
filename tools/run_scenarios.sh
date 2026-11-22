#!/usr/bin/env bash
# Runs every Wokwi scenario and checks its full log against the .expect file.
# Needs WOKWI_CLI_TOKEN. Usage: tools/run_scenarios.sh [scenario ...]   (default: all)
# Writes sim/wokwi/logs/<name>.log and sim/wokwi/logs/summary.txt; exit 0 only if all pass.
set -u
cd "$(dirname "$0")/.."
root=$PWD
proj=sim/wokwi
mkdir -p $proj/logs
names=("$@")
if [ ${#names[@]} -eq 0 ]; then
  for f in $proj/scenarios/*.yaml; do names+=("$(basename "$f" .yaml)"); done
fi
summary=$proj/logs/summary.txt
echo "Wokwi scenario run $(date -u +%Y-%m-%dT%H:%M:%SZ) commit $(git rev-parse --short HEAD)" > "$summary"
fail=0
for n in "${names[@]}"; do
  log=$proj/logs/$n.log
  start=$(date +%s)
  # --timeout is simulated time: 600 s at x60 = 10 hours of device time
  wokwi-cli $proj --scenario scenarios/$n.yaml --timeout 600000 > "$log" 2>&1
  rc=$?
  secs=$(( $(date +%s) - start ))
  if [ $rc -ne 0 ]; then
    echo "FAIL $n: simulator exit $rc after ${secs}s (scenario step not reached or timeout)" | tee -a "$summary"
    echo "    events and messages (sample lines omitted):"
    grep -v "^LOG S," "$log" | grep -vE "^(ets |rst:|configsip|clk_drv|mode:|load:|ho |entry )" | tail -60 | sed 's/^/    /'
    tail -5 "$log" | sed 's/^/    /' >> "$summary"
    fail=1
    continue
  fi
  python3 tools/check_scenario.py $proj/scenarios/$n.expect "$log" > $proj/logs/$n.check
  cat $proj/logs/$n.check | tee -a "$summary"
  if grep -q "^PASS" $proj/logs/$n.check; then
    echo "  ($n ran in ${secs}s)" >> "$summary"
  else
    fail=1
    echo "---- $n: events and messages (sample lines omitted)"; grep -v "^LOG S," "$log" | tail -60 | sed 's/^/    /'
  fi
done
echo "----" >> "$summary"
[ $fail -eq 0 ] && echo "ALL SCENARIOS PASSED" | tee -a "$summary" || echo "SCENARIOS FAILED" | tee -a "$summary"
exit $fail
