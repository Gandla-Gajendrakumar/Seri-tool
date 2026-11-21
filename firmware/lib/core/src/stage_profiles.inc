// ============================================================================================
//  STAGE PROFILES — the ONLY place these numbers live (requirement F-PROF-01).
//  Units: tenths. 250 = 25.0 C, 800 = 80 %RH.
//  Any change needs: (1) a cited source, (2) product-owner approval, (3) test_profiles update.
//
//  PROFILE(stage, key, tMin, tMax, rhMin, rhMax, source)
// ============================================================================================
PROFILE(Incubation, "incubation", 250, 260, 800, 800,
        "Product brief 2026-11-21; pending check against CSB / Chinese sources")
PROFILE(Chawki,     "chawki",     270, 280, 800, 900,
        "Product brief 2026-11-21; pending check against CSB / Chinese sources")
PROFILE(Instar3,    "instar3",    260, 260, 800, 800,
        "Product brief 2026-11-21; pending check against CSB / Chinese sources")
PROFILE(Instar4,    "instar4",    250, 250, 700, 700,
        "Product brief 2026-11-21; pending check against CSB / Chinese sources")
PROFILE(Instar5,    "instar5",    240, 240, 700, 700,
        "Product brief 2026-11-21; pending check against CSB / Chinese sources")
