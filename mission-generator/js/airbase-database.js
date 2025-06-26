// Comprehensive Military Airbase Database
const MILITARY_AIRBASES = {
    // === UNITED STATES ===
    // Atlantic Coast
    'KNHK': { 
        name: 'NAS Patuxent River', 
        country: 'USA', 
        region: 'Atlantic',
        aircraft: ['P-8A Poseidon', 'P-3C Orion'], 
        coords: [38.286, -76.412],
        description: 'Naval Air Systems Command test center'
    },
    'KNIP': { 
        name: 'NAS Jacksonville', 
        country: 'USA', 
        region: 'Atlantic',
        aircraft: ['P-8A Poseidon', 'EP-3E Aries'], 
        coords: [30.236, -81.876],
        description: 'Primary East Coast maritime patrol base'
    },
    'KNSE': { 
        name: 'NAS Oceana', 
        country: 'USA', 
        region: 'Atlantic',
        aircraft: ['F/A-18E/F Super Hornet'], 
        coords: [36.821, -76.033],
        description: 'Master Jet Base for East Coast'
    },
    'KNGP': { 
        name: 'NAS Pensacola', 
        country: 'USA', 
        region: 'Gulf_of_Mexico',
        aircraft: ['T-6A Texan II', 'T-45 Goshawk'], 
        coords: [30.352, -87.320],
        description: 'Cradle of Naval Aviation'
    },
    
    // Pacific Coast
    'PHIK': { 
        name: 'NAS Kaneohe Bay', 
        country: 'USA', 
        region: 'Pacific',
        aircraft: ['P-8A Poseidon', 'MV-22 Osprey'], 
        coords: [21.450, -157.768],
        description: 'Pacific maritime patrol operations'
    },
    'PHNL': { 
        name: 'Hickam AFB', 
        country: 'USA', 
        region: 'Pacific',
        aircraft: ['C-17A Globemaster', 'KC-135 Stratotanker'], 
        coords: [21.319, -157.922],
        description: 'Pacific Air Forces headquarters'
    },
    'KNKX': { 
        name: 'NAS Miramar', 
        country: 'USA', 
        region: 'Pacific',
        aircraft: ['F/A-18C/D Hornet', 'F-35C Lightning II'], 
        coords: [32.868, -117.142],
        description: 'Marine Corps Air Station'
    },
    'KNUW': { 
        name: 'NAS Whidbey Island', 
        country: 'USA', 
        region: 'Pacific',
        aircraft: ['P-8A Poseidon', 'EA-18G Growler'], 
        coords: [48.351, -122.656],
        description: 'Electronic attack and maritime patrol hub'
    },

    // === UNITED KINGDOM ===
    'EGPK': { 
        name: 'RAF Kinloss', 
        country: 'UK', 
        region: 'North_Sea',
        aircraft: ['P-8A Poseidon'], 
        coords: [57.649, -3.560],
        description: 'Maritime patrol aircraft base'
    },
    'EGQL': { 
        name: 'RAF Lossiemouth', 
        country: 'UK', 
        region: 'North_Sea',
        aircraft: ['Typhoon FGR4', 'P-8A Poseidon'], 
        coords: [57.705, -3.323],
        description: 'Quick reaction alert and maritime patrol'
    },
    'EGVA': { 
        name: 'RAF Fairford', 
        country: 'UK', 
        region: 'English_Channel',
        aircraft: ['B-52H Stratofortress', 'KC-135 Stratotanker'], 
        coords: [51.682, -1.790],
        description: 'US strategic bomber forward operating base'
    },
    'EGUL': { 
        name: 'RAF Lakenheath', 
        country: 'UK', 
        region: 'North_Sea',
        aircraft: ['F-15E Strike Eagle', 'F-35A Lightning II'], 
        coords: [52.409, 0.561],
        description: 'USAFE fighter operations'
    },
    'EGYM': { 
        name: 'RAF Marham', 
        country: 'UK', 
        region: 'North_Sea',
        aircraft: ['F-35B Lightning II'], 
        coords: [52.648, 0.550],
        description: 'Lightning Force headquarters'
    },
    'EGXC': { 
        name: 'RAF Coningsby', 
        country: 'UK', 
        region: 'North_Sea',
        aircraft: ['Typhoon FGR4'], 
        coords: [53.093, -0.166],
        description: 'Typhoon operations and training'
    },

    // === FRANCE ===
    'LFRB': { 
        name: 'BAN Brest-Lanvéoc', 
        country: 'France', 
        region: 'Atlantic',
        aircraft: ['Atlantique 2', 'Dauphin'], 
        coords: [48.281, -4.456],
        description: 'French Navy maritime patrol base'
    },
    'LFTH': { 
        name: 'BAN Hyères', 
        country: 'France', 
        region: 'Mediterranean',
        aircraft: ['Atlantique 2', 'Falcon 50M'], 
        coords: [43.097, 6.146],
        description: 'Mediterranean maritime surveillance'
    },
    'LFSI': { 
        name: 'BA 113 Saint-Dizier', 
        country: 'France', 
        region: 'English_Channel',
        aircraft: ['Rafale B/C'], 
        coords: [48.636, 4.899],
        description: 'Rafale fighter base'
    },
    'LFSO': { 
        name: 'BA 118 Mont-de-Marsan', 
        country: 'France', 
        region: 'Atlantic',
        aircraft: ['Rafale B/C', 'Mirage 2000D'], 
        coords: [43.909, -0.507],
        description: 'Fighter and attack aircraft base'
    },
    'LFOA': { 
        name: 'BA 705 Tours', 
        country: 'France', 
        region: 'English_Channel',
        aircraft: ['C-135FR', 'C-160 Transall'], 
        coords: [47.432, 0.728],
        description: 'Air transport and refueling'
    },

    // === GERMANY ===
    'EDXF': { 
        name: 'Nordholz Naval Airbase', 
        country: 'Germany', 
        region: 'North_Sea',
        aircraft: ['P-3C Orion', 'Sea King'], 
        coords: [53.768, 8.658],
        description: 'German Navy maritime patrol'
    },
    'ETNW': { 
        name: 'Wittmund AB', 
        country: 'Germany', 
        region: 'North_Sea',
        aircraft: ['Typhoon'], 
        coords: [53.548, 7.667],
        description: 'Air defense and QRA'
    },
    'ETNG': { 
        name: 'Geilenkirchen AB', 
        country: 'Germany', 
        region: 'North_Sea',
        aircraft: ['E-3A Sentry'], 
        coords: [50.961, 6.042],
        description: 'NATO AWACS base'
    },
    'ETAR': { 
        name: 'Ramstein AB', 
        country: 'Germany', 
        region: 'English_Channel',
        aircraft: ['C-130J Super Hercules', 'C-17A Globemaster'], 
        coords: [49.437, 7.600],
        description: 'USAFE headquarters and airlift hub'
    },
    'ETAD': { 
        name: 'Spangdahlem AB', 
        country: 'Germany', 
        region: 'English_Channel',
        aircraft: ['F-16C/D Fighting Falcon'], 
        coords: [49.973, 6.692],
        description: 'USAFE fighter operations'
    },

    // === NETHERLANDS ===
    'EHVK': { 
        name: 'Volkel AB', 
        country: 'Netherlands', 
        region: 'North_Sea',
        aircraft: ['F-16AM/BM Fighting Falcon'], 
        coords: [51.656, 5.709],
        description: 'Primary fighter base'
    },
    'EHLW': { 
        name: 'Leeuwarden AB', 
        country: 'Netherlands', 
        region: 'North_Sea',
        aircraft: ['F-16AM/BM Fighting Falcon'], 
        coords: [53.229, 5.761],
        description: 'Fighter operations and training'
    },
    'EHGR': { 
        name: 'Gilze-Rijen AB', 
        country: 'Netherlands', 
        region: 'North_Sea',
        aircraft: ['C-130H Hercules', 'CH-47D Chinook'], 
        coords: [51.567, 4.932],
        description: 'Transport and helicopter operations'
    },

    // === ITALY ===
    'LICA': { 
        name: 'NAS Catania-Fontanarossa', 
        country: 'Italy', 
        region: 'Mediterranean',
        aircraft: ['ATR 72MP', 'AV-8B+ Harrier II'], 
        coords: [37.467, 15.066],
        description: 'Italian Navy air station'
    },
    'LIPR': { 
        name: 'NAS Pratica di Mare', 
        country: 'Italy', 
        region: 'Mediterranean',
        aircraft: ['ATR 72MP', 'C-27J Spartan'], 
        coords: [41.652, 12.444],
        description: 'Italian Air Force coastal command'
    },
    'LIRN': { 
        name: 'NAS Naples (Capodichino)', 
        country: 'Italy', 
        region: 'Mediterranean',
        aircraft: ['P-8A Poseidon', 'C-40A Clipper'], 
        coords: [40.886, 14.291],
        description: 'US Navy support facility'
    },
    'LIRS': { 
        name: 'NAS Sigonella', 
        country: 'Italy', 
        region: 'Mediterranean',
        aircraft: ['P-8A Poseidon', 'MQ-4C Triton'], 
        coords: [37.402, 14.922],
        description: 'Strategic maritime patrol hub'
    },

    // === SPAIN ===
    'LERT': { 
        name: 'Rota Naval Station', 
        country: 'Spain', 
        region: 'Atlantic',
        aircraft: ['P-8A Poseidon', 'KC-130J Super Hercules'], 
        coords: [36.645, -6.349],
        description: 'US/Spanish maritime operations'
    },
    'LEZL': { 
        name: 'Zaragoza AB', 
        country: 'Spain', 
        region: 'Mediterranean',
        aircraft: ['C-130H Hercules', 'KC-130H'], 
        coords: [41.666, -1.042],
        description: 'Air transport and refueling'
    },

    // === NORWAY ===
    'ENAT': { 
        name: 'Andøya Air Station', 
        country: 'Norway', 
        region: 'Arctic',
        aircraft: ['P-3C Orion', 'P-8A Poseidon'], 
        coords: [69.293, 16.144],
        description: 'Arctic maritime patrol operations'
    },
    'ENBR': { 
        name: 'Bergen Airport Flesland', 
        country: 'Norway', 
        region: 'North_Sea',
        aircraft: ['Sea King', 'NH90'], 
        coords: [60.294, 5.218],
        description: 'Search and rescue operations'
    },
    'ENTC': { 
        name: 'Tromsø Airport', 
        country: 'Norway', 
        region: 'Arctic',
        aircraft: ['F-16AM/BM Fighting Falcon'], 
        coords: [69.683, 18.919],
        description: 'Arctic air operations'
    },

    // === PORTUGAL ===
    'LPMT': { 
        name: 'Montijo Air Base', 
        country: 'Portugal', 
        region: 'Atlantic',
        aircraft: ['P-3C Orion', 'C-295M'], 
        coords: [38.703, -8.973],
        description: 'Maritime patrol and transport'
    },
    'LPLA': { 
        name: 'Lajes Field', 
        country: 'Portugal', 
        region: 'Atlantic',
        aircraft: ['KC-10A Extender', 'C-130J Super Hercules'], 
        coords: [38.762, -27.091],
        description: 'Atlantic refueling and transport hub'
    },

    // === CANADA ===
    'CYAW': { 
        name: '14 Wing Greenwood', 
        country: 'Canada', 
        region: 'Atlantic',
        aircraft: ['CP-140 Aurora'], 
        coords: [44.984, -64.917],
        description: 'Atlantic maritime patrol operations'
    },
    'CYCO': { 
        name: '19 Wing Comox', 
        country: 'Canada', 
        region: 'Pacific',
        aircraft: ['CP-140 Aurora'], 
        coords: [49.711, -124.887],
        description: 'Pacific maritime patrol operations'
    },
    'CYBG': { 
        name: 'CFB Bagotville', 
        country: 'Canada', 
        region: 'Atlantic',
        aircraft: ['CF-188 Hornet'], 
        coords: [48.331, -70.996],
        description: 'NORAD air defense'
    },

    // === AUSTRALIA ===
    'YPED': { 
        name: 'RAAF Base Edinburgh', 
        country: 'Australia', 
        region: 'Pacific',
        aircraft: ['P-8A Poseidon', 'KC-30A MRTT'], 
        coords: [-34.702, 138.621],
        description: 'Maritime patrol and air-to-air refueling'
    },
    'YWLM': { 
        name: 'RAAF Base Williamtown', 
        country: 'Australia', 
        region: 'Pacific',
        aircraft: ['F/A-18F Super Hornet', 'E-7A Wedgetail'], 
        coords: [-32.792, 151.841],
        description: 'Fighter operations and AEW&C'
    },

    // === JAPAN ===
    'RJOI': { 
        name: 'Iwakuni AB', 
        country: 'Japan', 
        region: 'Pacific',
        aircraft: ['F/A-18C/D Hornet', 'F-35B Lightning II'], 
        coords: [34.144, 132.236],
        description: 'US Marine Corps air station'
    },
    'RJTY': { 
        name: 'Yokota AB', 
        country: 'Japan', 
        region: 'Pacific',
        aircraft: ['C-130J Super Hercules', 'UH-1N Huey'], 
        coords: [35.748, 139.348],
        description: 'PACAF headquarters and airlift'
    },

    // === SOUTH KOREA ===
    'RKSO': { 
        name: 'Osan AB', 
        country: 'South Korea', 
        region: 'Pacific',
        aircraft: ['F-16C/D Fighting Falcon', 'A-10C Thunderbolt II'], 
        coords: [37.090, 127.030],
        description: '7th Air Force operations'
    },
    'RKJK': { 
        name: 'Kunsan AB', 
        country: 'South Korea', 
        region: 'Pacific',
        aircraft: ['F-16C/D Fighting Falcon'], 
        coords: [35.904, 126.616],
        description: 'Wolf Pack fighter operations'
    },

    // === ICELAND ===
    'BIKF': { 
        name: 'Keflavik Air Base', 
        country: 'Iceland', 
        region: 'Arctic',
        aircraft: ['P-8A Poseidon', 'KC-135 Stratotanker'], 
        coords: [63.985, -22.605],
        description: 'NATO maritime patrol operations'
    },

    // === TURKEY ===
    'LTAG': { 
        name: 'Incirlik AB', 
        country: 'Turkey', 
        region: 'Mediterranean',
        aircraft: ['F-16C/D Fighting Falcon', 'KC-135 Stratotanker'], 
        coords: [37.002, 35.426],
        description: 'NATO southern flank operations'
    },
    'LTBM': { 
        name: 'Merzifon AB', 
        country: 'Turkey', 
        region: 'Mediterranean',
        aircraft: ['F-16C/D Fighting Falcon'], 
        coords: [40.828, 35.522],
        description: 'Central Anatolia air defense'
    },

    // === GREECE ===
    'LGSO': { 
        name: 'Souda Bay', 
        country: 'Greece', 
        region: 'Mediterranean',
        aircraft: ['P-8A Poseidon', 'C-130H Hercules'], 
        coords: [35.546, 24.127],
        description: 'Eastern Mediterranean operations'
    },
    'LGTT': { 
        name: 'Tanagra AB', 
        country: 'Greece', 
        region: 'Mediterranean',
        aircraft: ['F-16C/D Fighting Falcon', 'Mirage 2000'], 
        coords: [38.344, 23.565],
        description: 'Hellenic Air Force fighter operations'
    },
    'LGSM': { 
        name: 'Kastelli AB', 
        country: 'Greece', 
        region: 'Mediterranean',
        aircraft: ['F-16C/D Fighting Falcon'], 
        coords: [35.421, 25.327],
        description: 'Aegean Sea air patrol'
    },

    // === SWEDEN ===
    'ESGG': { 
        name: 'Säve Air Base', 
        country: 'Sweden', 
        region: 'North_Sea',
        aircraft: ['JAS 39 Gripen', 'C-130H Hercules'], 
        coords: [57.774, 11.870],
        description: 'West coast air defense'
    },
    'ESPA': { 
        name: 'Luleå Air Base', 
        country: 'Sweden', 
        region: 'Arctic',
        aircraft: ['JAS 39 Gripen'], 
        coords: [65.548, 22.122],
        description: 'Arctic air surveillance'
    },
    'ESSA': { 
        name: 'Stockholm Arlanda', 
        country: 'Sweden', 
        region: 'North_Sea',
        aircraft: ['JAS 39 Gripen', 'S 102B Korpen'], 
        coords: [59.651, 17.918],
        description: 'Capital region air defense'
    },

    // === FINLAND ===
    'EFTU': { 
        name: 'Turku Air Base', 
        country: 'Finland', 
        region: 'Arctic',
        aircraft: ['F/A-18C/D Hornet'], 
        coords: [60.514, 22.262],
        description: 'Southwestern Finland air defense'
    },
    'EFRV': { 
        name: 'Rovaniemi Air Base', 
        country: 'Finland', 
        region: 'Arctic',
        aircraft: ['F/A-18C/D Hornet'], 
        coords: [66.564, 25.830],
        description: 'Arctic Circle air operations'
    },
    'EFTP': { 
        name: 'Tampere-Pirkkala', 
        country: 'Finland', 
        region: 'Arctic',
        aircraft: ['F/A-18C/D Hornet', 'C-295M'], 
        coords: [61.414, 23.604],
        description: 'Central Finland air base'
    },

    // === DENMARK ===
    'EKKA': { 
        name: 'Karup Air Base', 
        country: 'Denmark', 
        region: 'North_Sea',
        aircraft: ['F-16AM/BM Fighting Falcon'], 
        coords: [56.297, 9.124],
        description: 'Primary Danish fighter base'
    },
    'EKAH': { 
        name: 'Aalborg Air Base', 
        country: 'Denmark', 
        region: 'North_Sea',
        aircraft: ['F-35A Lightning II'], 
        coords: [57.092, 9.849],
        description: 'Northern Jutland air operations'
    },

    // === POLAND ===
    'EPKS': { 
        name: 'Krzesiny Air Base', 
        country: 'Poland', 
        region: 'North_Sea',
        aircraft: ['F-16C/D Fighting Falcon'], 
        coords: [52.331, 16.833],
        description: 'Western Poland air defense'
    },
    'EPML': { 
        name: 'Malbork Air Base', 
        country: 'Poland', 
        region: 'North_Sea',
        aircraft: ['F-16C/D Fighting Falcon'], 
        coords: [54.026, 19.134],
        description: 'Baltic coast air operations'
    },
    'EPDE': { 
        name: 'Deblin Air Base', 
        country: 'Poland', 
        region: 'North_Sea',
        aircraft: ['TS-11 Iskra', 'PZL-130 Orlik'], 
        coords: [51.552, 21.893],
        description: 'Polish Air Force Academy'
    },

    // === CZECH REPUBLIC ===
    'LKCV': { 
        name: 'Čáslav Air Base', 
        country: 'Czech Republic', 
        region: 'English_Channel',
        aircraft: ['JAS 39 Gripen'], 
        coords: [49.937, 15.381],
        description: 'Central Europe air defense'
    },
    'LKNA': { 
        name: 'Náměšť Air Base', 
        country: 'Czech Republic', 
        region: 'English_Channel',
        aircraft: ['JAS 39 Gripen'], 
        coords: [49.094, 16.125],
        description: 'Southern Czech air operations'
    },

    // === HUNGARY ===
    'LHKE': { 
        name: 'Kecskemét Air Base', 
        country: 'Hungary', 
        region: 'English_Channel',
        aircraft: ['JAS 39 Gripen'], 
        coords: [46.917, 19.749],
        description: 'Hungarian air defense hub'
    },
    'LHPA': { 
        name: 'Pápa Air Base', 
        country: 'Hungary', 
        region: 'English_Channel',
        aircraft: ['C-17A Globemaster III'], 
        coords: [47.364, 17.500],
        description: 'NATO strategic airlift'
    },

    // === ROMANIA ===
    'LRCV': { 
        name: 'Câmpia Turzii', 
        country: 'Romania', 
        region: 'Mediterranean',
        aircraft: ['F-16AM/BM Fighting Falcon'], 
        coords: [46.504, 23.885],
        description: 'Transylvanian air defense'
    },
    'LRBC': { 
        name: 'Bacău Air Base', 
        country: 'Romania', 
        region: 'Mediterranean',
        aircraft: ['MiG-21 LanceR'], 
        coords: [46.521, 26.910],
        description: 'Eastern Romania air operations'
    },

    // === BULGARIA ===
    'LBPG': { 
        name: 'Graf Ignatievo Air Base', 
        country: 'Bulgaria', 
        region: 'Mediterranean',
        aircraft: ['F-16C/D Fighting Falcon'], 
        coords: [42.288, 25.653],
        description: 'Central Bulgaria air defense'
    },

    // === CROATIA ===
    'LDPL': { 
        name: 'Pleso Air Base', 
        country: 'Croatia', 
        region: 'Mediterranean',
        aircraft: ['PC-9M', 'Mi-8/17'], 
        coords: [45.742, 16.068],
        description: 'Croatian Air Force headquarters'
    },

    // === SLOVENIA ===
    'LJCE': { 
        name: 'Cerklje ob Krki', 
        country: 'Slovenia', 
        region: 'Mediterranean',
        aircraft: ['PC-9M Hudournik'], 
        coords: [45.898, 15.530],
        description: 'Slovenian air operations'
    },

    // === BELGIUM ===
    'EBFS': { 
        name: 'Florennes Air Base', 
        country: 'Belgium', 
        region: 'English_Channel',
        aircraft: ['F-16AM/BM Fighting Falcon'], 
        coords: [50.243, 4.646],
        description: 'Belgian air defense'
    },
    'EBBL': { 
        name: 'Kleine Brogel', 
        country: 'Belgium', 
        region: 'English_Channel',
        aircraft: ['F-16AM/BM Fighting Falcon'], 
        coords: [51.168, 5.470],
        description: 'NATO tactical fighter wing'
    },

    // === LUXEMBOURG ===
    'ELLX': { 
        name: 'Luxembourg Airport', 
        country: 'Luxembourg', 
        region: 'English_Channel',
        aircraft: ['A400M Atlas'], 
        coords: [49.623, 6.204],
        description: 'NATO airlift coordination'
    },

    // === SWITZERLAND ===
    'LSMP': { 
        name: 'Payerne Air Base', 
        country: 'Switzerland', 
        region: 'English_Channel',
        aircraft: ['F/A-18C/D Hornet'], 
        coords: [46.843, 6.915],
        description: 'Swiss air policing'
    },
    'LSMM': { 
        name: 'Meiringen Air Base', 
        country: 'Switzerland', 
        region: 'English_Channel',
        aircraft: ['F/A-18C/D Hornet'], 
        coords: [46.743, 8.108],
        description: 'Alpine air operations'
    },

    // === AUSTRIA ===
    'LOWZ': { 
        name: 'Zeltweg Air Base', 
        country: 'Austria', 
        region: 'English_Channel',
        aircraft: ['Eurofighter Typhoon'], 
        coords: [47.203, 14.744],
        description: 'Austrian air surveillance'
    },

    // === ISRAEL ===
    'LLBG': { 
        name: 'Nevatim Airbase', 
        country: 'Israel', 
        region: 'Mediterranean',
        aircraft: ['F-35I Adir', 'F-16I Sufa'], 
        coords: [31.208, 34.926],
        description: 'Southern command air operations'
    },
    'LLHS': { 
        name: 'Hatzerim Air Base', 
        country: 'Israel', 
        region: 'Mediterranean',
        aircraft: ['F-16I Sufa', 'AH-64 Apache'], 
        coords: [31.234, 34.665],
        description: 'Desert air combat training'
    },
    'LLET': { 
        name: 'Tel Nof Airbase', 
        country: 'Israel', 
        region: 'Mediterranean',
        aircraft: ['F-15I Ra\'am', 'C-130J Samson'], 
        coords: [31.838, 34.821],
        description: 'Central command operations'
    },

    // === INDIA ===
    'VOBL': { 
        name: 'INS Hansa Goa', 
        country: 'India', 
        region: 'Indian_Ocean',
        aircraft: ['P-8I Neptune', 'MiG-29K'], 
        coords: [15.381, 73.831],
        description: 'Indian Navy western fleet'
    },
    'VOCP': { 
        name: 'Port Blair', 
        country: 'India', 
        region: 'Indian_Ocean',
        aircraft: ['Dornier 228', 'Mi-8'], 
        coords: [11.641, 92.730],
        description: 'Andaman & Nicobar operations'
    },
    'VOKX': { 
        name: 'Kochi Naval Base', 
        country: 'India', 
        region: 'Indian_Ocean',
        aircraft: ['P-8I Neptune', 'Sea King'], 
        coords: [9.942, 76.267],
        description: 'Southern naval aviation'
    },

    // === SINGAPORE ===
    'WSAP': { 
        name: 'Paya Lebar Air Base', 
        country: 'Singapore', 
        region: 'Pacific',
        aircraft: ['F-15SG Strike Eagle', 'F-16C/D Fighting Falcon'], 
        coords: [1.360, 103.909],
        description: 'Republic of Singapore Air Force'
    },
    'WSSL': { 
        name: 'Sembawang Air Base', 
        country: 'Singapore', 
        region: 'Pacific',
        aircraft: ['C-130H Hercules', 'AS332 Super Puma'], 
        coords: [1.425, 103.813],
        description: 'Transport and SAR operations'
    },

    // === MALAYSIA ===
    'WMKD': { 
        name: 'Kuantan Air Base', 
        country: 'Malaysia', 
        region: 'Pacific',
        aircraft: ['F/A-18D Hornet', 'Hawk 208'], 
        coords: [3.775, 103.209],
        description: 'East coast air defense'
    },
    'WMKB': { 
        name: 'Butterworth Air Base', 
        country: 'Malaysia', 
        region: 'Pacific',
        aircraft: ['F/A-18D Hornet'], 
        coords: [5.465, 100.390],
        description: 'FPDA air operations'
    },

    // === THAILAND ===
    'VTBD': { 
        name: 'Don Muang Air Base', 
        country: 'Thailand', 
        region: 'Pacific',
        aircraft: ['F-16A/B Fighting Falcon', 'JAS 39 Gripen'], 
        coords: [13.912, 100.607],
        description: 'Royal Thai Air Force headquarters'
    },
    'VTBU': { 
        name: 'U-Tapao Air Base', 
        country: 'Thailand', 
        region: 'Pacific',
        aircraft: ['C-130H Hercules', 'Saab 340 AEW'], 
        coords: [12.680, 101.005],
        description: 'Maritime patrol and transport'
    },

    // === PHILIPPINES ===
    'RPLC': { 
        name: 'Clark Air Base', 
        country: 'Philippines', 
        region: 'Pacific',
        aircraft: ['FA-50 Fighting Eagle'], 
        coords: [15.186, 120.560],
        description: 'Philippine Air Force operations'
    },
    'RPLL': { 
        name: 'Villamor Air Base', 
        country: 'Philippines', 
        region: 'Pacific',
        aircraft: ['C-130T Hercules', 'UH-1H Huey'], 
        coords: [14.509, 121.020],
        description: 'Capital air defense'
    },

    // === INDONESIA ===
    'WIHH': { 
        name: 'Halim Perdanakusuma', 
        country: 'Indonesia', 
        region: 'Pacific',
        aircraft: ['F-16A/B Fighting Falcon', 'Su-27/30'], 
        coords: [-6.266, 106.891],
        description: 'Jakarta air defense'
    },
    'WIDD': { 
        name: 'Ngurah Rai Air Base', 
        country: 'Indonesia', 
        region: 'Pacific',
        aircraft: ['F-16A/B Fighting Falcon'], 
        coords: [-8.748, 115.167],
        description: 'Bali air operations'
    },

    // === NEW ZEALAND ===
    'NZOH': { 
        name: 'RNZAF Base Ohakea', 
        country: 'New Zealand', 
        region: 'Pacific',
        aircraft: ['C-130H Hercules', 'P-3K2 Orion'], 
        coords: [-40.206, 175.388],
        description: 'Transport and maritime patrol'
    },
    'NZWN': { 
        name: 'RNZAF Base Woodbourne', 
        country: 'New Zealand', 
        region: 'Pacific',
        aircraft: ['King Air 350', 'NH90'], 
        coords: [-41.518, 173.870],
        description: 'South Island operations'
    },

    // === SOUTH AFRICA ===
    'FAOY': { 
        name: 'Ysterplaat AFB', 
        country: 'South Africa', 
        region: 'Atlantic',
        aircraft: ['C-130BZ Hercules', 'Casa 212'], 
        coords: [-33.900, 18.498],
        description: 'Cape Town maritime patrol'
    },
    'FAOR': { 
        name: 'OR Tambo International', 
        country: 'South Africa', 
        region: 'Indian_Ocean',
        aircraft: ['A400M Atlas', 'C-47TP Turbo Dakota'], 
        coords: [-26.139, 28.246],
        description: 'Central command operations'
    },
    'FADN': { 
        name: 'Durban Air Station', 
        country: 'South Africa', 
        region: 'Indian_Ocean',
        aircraft: ['Lynx Mk7', 'Casa 212'], 
        coords: [-29.970, 30.951],
        description: 'East coast maritime operations'
    },

    // === CHILE ===
    'SCEL': { 
        name: 'Los Cerrillos Air Base', 
        country: 'Chile', 
        region: 'Pacific',
        aircraft: ['F-16C/D Fighting Falcon'], 
        coords: [-33.490, -70.704],
        description: 'Santiago air defense'
    },
    'SCIP': { 
        name: 'Iquique Air Base', 
        country: 'Chile', 
        region: 'Pacific',
        aircraft: ['F-16C/D Fighting Falcon'], 
        coords: [-20.565, -70.181],
        description: 'Northern Chile operations'
    },
    'SCTE': { 
        name: 'Teniente Julio Gallardo', 
        country: 'Chile', 
        region: 'Pacific',
        aircraft: ['C-130H Hercules'], 
        coords: [-53.779, -70.856],
        description: 'Patagonian operations'
    },

    // === BRAZIL ===
    'SBNT': { 
        name: 'Natal Air Base', 
        country: 'Brazil', 
        region: 'Atlantic',
        aircraft: ['P-3AM Orion', 'EMB-111'], 
        coords: [-5.911, -35.247],
        description: 'Atlantic maritime patrol'
    },
    'SBAN': { 
        name: 'Anápolis Air Base', 
        country: 'Brazil', 
        region: 'Atlantic',
        aircraft: ['F-5EM Tiger II', 'A-29 Super Tucano'], 
        coords: [-16.223, -48.996],
        description: 'Central Brazil air defense'
    },
    'SBSV': { 
        name: 'Salvador Air Base', 
        country: 'Brazil', 
        region: 'Atlantic',
        aircraft: ['A-29 Super Tucano'], 
        coords: [-12.911, -38.322],
        description: 'Northeast coast operations'
    },

    // === ARGENTINA ===
    'SAEZ': { 
        name: 'Ezeiza Air Base', 
        country: 'Argentina', 
        region: 'Atlantic',
        aircraft: ['A-4AR Fightinghawk'], 
        coords: [-34.822, -58.536],
        description: 'Buenos Aires air operations'
    },
    'SAWC': { 
        name: 'Comodoro Rivadavia', 
        country: 'Argentina', 
        region: 'Atlantic',
        aircraft: ['IA-58 Pucará'], 
        coords: [-45.785, -67.465],
        description: 'Patagonian coastal patrol'
    },

    // === MOROCCO ===
    'GMMN': { 
        name: 'Sale Air Base', 
        country: 'Morocco', 
        region: 'Atlantic',
        aircraft: ['F-16C/D Fighting Falcon'], 
        coords: [34.051, -6.751],
        description: 'Royal Moroccan Air Force'
    },

    // === EGYPT ===
    'HEAX': { 
        name: 'Alexandria Air Base', 
        country: 'Egypt', 
        region: 'Mediterranean',
        aircraft: ['F-16C/D Fighting Falcon'], 
        coords: [31.184, 29.949],
        description: 'Mediterranean coast operations'
    },
    'HECA': { 
        name: 'Cairo West Air Base', 
        country: 'Egypt', 
        region: 'Mediterranean',
        aircraft: ['F-16C/D Fighting Falcon', 'Rafale'], 
        coords: [30.115, 30.916],
        description: 'Capital air defense'
    },

    // === UAE ===
    'OMAM': { 
        name: 'Al Minhad Air Base', 
        country: 'UAE', 
        region: 'Indian_Ocean',
        aircraft: ['F-16E/F Desert Falcon'], 
        coords: [25.027, 55.366],
        description: 'Coalition air operations'
    },
    'OMAA': { 
        name: 'Al Dhafra Air Base', 
        country: 'UAE', 
        region: 'Indian_Ocean',
        aircraft: ['F-35A Lightning II', 'Rafale'], 
        coords: [24.248, 54.547],
        description: 'Western region air defense'
    },

    // === SAUDI ARABIA ===
    'OEPR': { 
        name: 'Prince Sultan Air Base', 
        country: 'Saudi Arabia', 
        region: 'Indian_Ocean',
        aircraft: ['F-15SA Eagle', 'Typhoon'], 
        coords: [24.069, 47.680],
        description: 'Central command operations'
    },
    'OERY': { 
        name: 'King Faisal Air Base', 
        country: 'Saudi Arabia', 
        region: 'Indian_Ocean',
        aircraft: ['F-15SA Eagle'], 
        coords: [27.900, 45.528],
        description: 'Northern border operations'
    }
};

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { MILITARY_AIRBASES };
}