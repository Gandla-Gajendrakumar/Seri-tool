# Stage profile sources

Checked 2026-11-21. Values live in `firmware/lib/core/src/stage_profiles.inc`. Quotes below were
read from the fetched page text (not from search snippets) on that date.

## Verified against the document

| Stage | Brief value | Source | Exact quote |
|---|---|---|---|
| Incubation | 25-26 C, 80 % | **[CSB-PUNE]** Central Silk Board, Regional Sericultural Research Station Pune, "Rearing of Mulberry Silkworm", section *Incubation and brushing* | "Maintain the room temperature at 25-26°C and relative humidity at 80%." |
| Chawki I-II | 27-28 C, 80-90 % | **[CSB-PUNE]** section *Chawki rearing* | "Maintain temperature at 27-28°C and relative humidity at 80-90%." |
| Instar III | 26 C, 80 % | **[VIK-CSRTI]** Vikaspedia (Govt. of India portal), "Silkworm rearing", section *Maintenance of temperature and humidity* | "The ideal temperature for the late age rearing is 26 C for III instar larvae, 25 C for IV instar and 24 C for V instar larvae. 80% humidity is required for III instar larvae and 70% is required for IV and V instar larvae." |
| Instar IV | 25 C, 70 % | **[VIK-CSRTI]** (same sentence) | as above |
| Instar V | 24 C, 70 % | **[VIK-CSRTI]** (same sentence) | as above |

- [CSB-PUNE] https://silks.csb.gov.in/pune/?p=12
- [VIK-CSRTI] https://en.vikaspedia.in/viewcontent/agriculture/farm-based-enterprises/sericulture/silkworm-rearing
- [CSB-BELGAUM] https://silks.csb.gov.in/belgaum/?p=230

**Result: all five brief values are confirmed. No value change.**

## Differences found. Decided by product owner 2026-11-22: all three recommendations accepted

Applied: (1) incubation RH target widened to **70-80 %**; (2) new **Spinning** stage **24-28 C, 60-70 %** covering both sources; (3) **no moult mode** in V1, because no source gives a figure.


1. **Incubation humidity.** [CSB-BELGAUM] says "Incubation eggs Maintain temperature of 25 ± 1˚C,
   relative humidity of 75 ± 5%, 16 hours of light and 8 hours darkness." That gives a target of
   70-80 %. Pune says 80 %. With our ±5 % tolerance, the brief's 80 % alarms below 75 %, while
   Belgaum's would alarm below 65 %.
2. **Spinning / mounting stage is missing from the brief.** The two sources disagree on temperature:
   - [CSB-PUNE] *Mounting and Harvesting*: "Maintain temperature of 27-28°C and RH of 60-70%."
   - [VIK-CSRTI]: "When the larvae are in spinning stage, the room temperature of 24°C and 60-70 % of
     relative humidity along with good aeration facility are to be provided."
   - Secondary (search snippet only, not fetched): FAO silk reeling manual x2099e, "at or near 25ºC
     and relative humidity around 65 percent".
3. **Moulting.** Both CSB pages say to keep the bed dry during moults: "Spread the bed for proper
   drying and aeration. Dust lime powder on the bed to keep the humidity low." **Neither gives a
   number.** Without a number there is nothing to base a moult alarm band on.

## Could not verify (search snippets only)

Chinese standards (Jiangxi DB36/T 669-2024 was found, but no figures could be read), FAO Bulletin
73/2 *Silkworm rearing* (fao.org fetch failed), per-instar durations for CSR hybrids. A Japanese
table (MLIT) gives 1-2 °C cooler late-age figures for temperate races. Not used, because Indian
CSR hybrids are reared to the Indian figures.
