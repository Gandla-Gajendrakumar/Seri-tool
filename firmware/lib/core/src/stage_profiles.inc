// ============================================================================================
//  STAGE PROFILES — the ONLY place these numbers live (requirement F-PROF-01).
//  Units: tenths. 250 = 25.0 C, 800 = 80 %RH.
//  Any change needs: (1) a cited source, (2) product-owner approval, (3) test_profiles update.
//
//  Full quotes and differences: docs/stage_profile_sources.md
//
//  PROFILE(stage, key, tMin, tMax, rhMin, rhMax, source)
// ============================================================================================
PROFILE(Incubation, "incubation", 250, 260, 800, 800,
        "CSB RSRS Pune, Rearing of Mulberry Silkworm: Incubation (silks.csb.gov.in/pune/?p=12)")
PROFILE(Chawki,     "chawki",     270, 280, 800, 900,
        "CSB RSRS Pune, Rearing of Mulberry Silkworm: Chawki rearing (silks.csb.gov.in/pune/?p=12)")
PROFILE(Instar3,    "instar3",    260, 260, 800, 800,
        "Vikaspedia Silkworm rearing: late age temp/humidity (en.vikaspedia.in ... /silkworm-rearing)")
PROFILE(Instar4,    "instar4",    250, 250, 700, 700,
        "Vikaspedia Silkworm rearing: late age temp/humidity (en.vikaspedia.in ... /silkworm-rearing)")
PROFILE(Instar5,    "instar5",    240, 240, 700, 700,
        "Vikaspedia Silkworm rearing: late age temp/humidity (en.vikaspedia.in ... /silkworm-rearing)")
