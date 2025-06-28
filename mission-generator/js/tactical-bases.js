// Tactical Bases Database
// Military and Naval bases worldwide for maritime patrol operations

const TACTICAL_BASES = {
    // United Kingdom - Maritime Patrol Bases
    "EGXN": {
        name: "RAF Kinloss",
        type: "AIR_FORCE", 
        country: "United Kingdom",
        coordinates: [57.6495, -3.5606],
        icao: "EGXN",
        capabilities: ["Maritime Patrol", "SAR", "Training"],
        aircraft: ["P-8A Poseidon"],
        runway: "08/26 - 1829m",
        frequency: "122.100",
        elevation: "21m"
    },
    "EGVN": {
        name: "RAF Brize Norton",
        type: "AIR_FORCE",
        country: "United Kingdom",
        coordinates: [51.7500, -1.5839],
        icao: "EGVN",
        capabilities: ["Strategic Transport", "Air Mobility", "Maritime Support"],
        aircraft: ["C-130J", "A400M", "Voyager"],
        runway: "07/25 - 3050m",
        frequency: "119.000",
        elevation: "88m"
    },
    "EGUN": {
        name: "RAF Mildenhall",
        type: "AIR_FORCE",
        country: "United Kingdom",
        coordinates: [52.3619, 0.4864],
        icao: "EGUN",
        capabilities: ["Maritime Patrol", "ISR", "SIGINT"],
        aircraft: ["RC-135", "U-2", "KC-135"],
        runway: "11/29 - 3048m",
        frequency: "121.750",
        elevation: "10m"
    },
    "EGXW": {
        name: "RAF Waddington",
        type: "AIR_FORCE",
        country: "United Kingdom",
        coordinates: [53.1661, -0.5238],
        icao: "EGXW",
        capabilities: ["ISR", "Maritime Patrol", "ISTAR"],
        aircraft: ["E-3D Sentry", "RC-135"],
        runway: "02/20 - 2743m",
        frequency: "120.300",
        elevation: "68m"
    },
    "EGQM": {
        name: "RAF Lossiemouth",
        type: "AIR_FORCE",
        country: "United Kingdom",
        coordinates: [57.7058, -3.3258],
        icao: "EGQM",
        capabilities: ["Maritime Patrol", "QRA", "Training"],
        aircraft: ["P-8A Poseidon", "Typhoon"],
        runway: "05/23 - 2745m",
        frequency: "119.350",
        elevation: "10m"
    },
    "EGXC": {
        name: "RAF Coningsby",
        type: "AIR_FORCE",
        country: "United Kingdom",
        coordinates: [53.0928, -0.1661],
        icao: "EGXC",
        capabilities: ["QRA", "Training", "Air Defense"],
        aircraft: ["Typhoon"],
        runway: "07/25 - 2743m",
        frequency: "120.775",
        elevation: "8m"
    },
    "EGXE": {
        name: "RAF Leeming",
        type: "AIR_FORCE",
        country: "United Kingdom",
        coordinates: [54.2922, -1.5358],
        icao: "EGXE",
        capabilities: ["Training", "Transport"],
        aircraft: ["Hawk T2", "King Air"],
        runway: "16/34 - 2286m",
        frequency: "122.100",
        elevation: "40m"
    },
    "EGYM": {
        name: "RAF Marham",
        type: "AIR_FORCE",
        country: "United Kingdom",
        coordinates: [52.6481, 0.5508],
        icao: "EGYM",
        capabilities: ["Strategic Operations", "Fast Jet Training"],
        aircraft: ["F-35B Lightning II"],
        runway: "06/24 - 2743m",
        frequency: "124.150",
        elevation: "23m"
    },
    "EGHH": {
        name: "RNAS Yeovilton",
        type: "NAVAL_AIR",
        country: "United Kingdom",
        coordinates: [51.0094, -2.6383],
        icao: "EGHH",
        capabilities: ["Naval Aviation", "Helicopter Ops", "Maritime Training"],
        aircraft: ["Wildcat", "Merlin"],
        runway: "04/22 - 1829m",
        frequency: "130.200",
        elevation: "23m"
    },
    "EGDY": {
        name: "RNAS Culdrose",
        type: "NAVAL_AIR",
        country: "United Kingdom",
        coordinates: [50.0856, -5.2558],
        icao: "EGDY",
        capabilities: ["Maritime SAR", "ASW Training", "Naval Aviation"],
        aircraft: ["Merlin", "Sea King"],
        runway: "12/30 - 1829m",
        frequency: "134.050",
        elevation: "80m"
    },
    
    // United States - NATO Maritime Patrol
    "KNHK": {
        name: "Patuxent River NAS",
        type: "NAVAL_AIR",
        country: "United States",
        coordinates: [38.2856, -76.4119],
        icao: "KNHK",
        capabilities: ["Maritime Patrol", "Test & Evaluation", "Atlantic Ops"],
        aircraft: ["P-8A Poseidon", "MQ-4C Triton"],
        runway: "06/24 - 3658m",
        frequency: "120.600",
        elevation: "14m"
    },
    "KNIP": {
        name: "Naval Air Station Jacksonville",
        type: "NAVAL_AIR",
        country: "United States",
        coordinates: [30.2358, -81.6806],
        icao: "KNIP",
        capabilities: ["Maritime Patrol", "ASW", "Atlantic Fleet"],
        aircraft: ["P-8A Poseidon", "SH-60"],
        runway: "09/27 - 2438m",
        frequency: "118.500",
        elevation: "8m"
    },
    "PHNL": {
        name: "Hickam Air Force Base",
        type: "AIR_FORCE",
        country: "United States",
        coordinates: [21.3186, -157.9225],
        icao: "PHNL",
        capabilities: ["Pacific Maritime Patrol", "Strategic Ops", "Airlift"],
        aircraft: ["KC-135", "C-17", "F-22"],
        runway: "08L/26R - 3749m",
        frequency: "118.100",
        elevation: "4m"
    },
    "KNTU": {
        name: "NAS Oceana",
        type: "NAVAL_AIR",
        country: "United States",
        coordinates: [36.8206, -76.0336],
        icao: "KNTU",
        capabilities: ["Maritime Patrol", "Carrier Ops", "Atlantic Fleet"],
        aircraft: ["F/A-18 Super Hornet", "EA-18G Growler"],
        runway: "05L/23R - 3658m",
        frequency: "126.200",
        elevation: "8m"
    },
    "KWRI": {
        name: "McGuire Air Force Base",
        type: "AIR_FORCE",
        country: "United States",
        coordinates: [40.0158, -74.5911],
        icao: "KWRI",
        capabilities: ["Maritime Patrol", "Airlift", "Refueling"],
        aircraft: ["KC-46", "C-17"],
        runway: "06/24 - 3048m",
        frequency: "118.050",
        elevation: "42m"
    },
    
    // Canada - NORAD/NATO Partner
    "CYYG": {
        name: "CFB Comox",
        type: "AIR_FORCE",
        country: "Canada",
        coordinates: [49.7108, -124.8867],
        icao: "CYYG",
        capabilities: ["Maritime Patrol", "SAR", "Pacific Ops"],
        aircraft: ["CP-140 Aurora", "CC-115 Buffalo"],
        runway: "11/29 - 3048m",
        frequency: "119.700",
        elevation: "21m"
    },
    "CYYR": {
        name: "CFB Goose Bay",
        type: "AIR_FORCE",
        country: "Canada",
        coordinates: [53.3194, -60.4258],
        icao: "CYYR",
        capabilities: ["Atlantic Maritime Patrol", "Arctic Ops", "Training"],
        aircraft: ["CP-140 Aurora"],
        runway: "08/26 - 3353m",
        frequency: "133.400",
        elevation: "49m"
    },
    "CYQY": {
        name: "CFB Shearwater",
        type: "NAVAL_AIR",
        country: "Canada",
        coordinates: [44.6386, -63.5061],
        icao: "CYQY",
        capabilities: ["Maritime Patrol", "ASW", "Atlantic Fleet"],
        aircraft: ["CH-148 Cyclone", "CP-140 Aurora"],
        runway: "16/34 - 1524m",
        frequency: "120.100",
        elevation: "51m"
    },
    
    // France - NATO Partner
    "LFRB": {
        name: "Brest Bretagne Airport",
        type: "NAVAL_AIR",
        country: "France",
        coordinates: [48.4478, -4.4186],
        icao: "LFRB",
        capabilities: ["Maritime Patrol", "Atlantic Ops", "ASW"],
        aircraft: ["Atlantique 2", "Falcon 50"],
        runway: "07/25 - 3180m",
        frequency: "120.500",
        elevation: "99m"
    },
    "LFTH": {
        name: "Hyères Le Palyvestre",
        type: "NAVAL_AIR",
        country: "France",
        coordinates: [43.0969, 6.1469],
        icao: "LFTH",
        capabilities: ["Mediterranean Patrol", "Training", "ASW"],
        aircraft: ["Atlantique 2", "Super Étendard"],
        runway: "04/22 - 2440m",
        frequency: "118.300",
        elevation: "4m"
    },
    "LFOA": {
        name: "Avord Air Base",
        type: "AIR_FORCE",
        country: "France",
        coordinates: [47.0533, 2.6389],
        icao: "LFOA",
        capabilities: ["Training", "Maritime Support", "Transport"],
        aircraft: ["Alpha Jet", "TBM 700"],
        runway: "09/27 - 2440m",
        frequency: "122.100",
        elevation: "183m"
    },
    
    // Germany - NATO Core
    "ETNN": {
        name: "Nordholz Naval Airbase",
        type: "NAVAL_AIR",
        country: "Germany",
        coordinates: [53.7653, 8.6581],
        icao: "ETNN",
        capabilities: ["Maritime Patrol", "North Sea Ops", "SAR"],
        aircraft: ["P-3C Orion", "Sea King"],
        runway: "06/24 - 2331m",
        frequency: "130.725",
        elevation: "21m"
    },
    "ETMN": {
        name: "Manching Air Base",
        type: "AIR_FORCE",
        country: "Germany",
        coordinates: [48.7158, 11.5342],
        icao: "ETMN",
        capabilities: ["Test & Evaluation", "Training"],
        aircraft: ["Tornado", "Eurofighter"],
        runway: "04/22 - 2500m",
        frequency: "118.775",
        elevation: "364m"
    },
    
    // Italy - NATO Partner
    "LIRF": {
        name: "Naval Air Station Sigonella",
        type: "NAVAL_AIR",
        country: "Italy/USA",
        coordinates: [37.4017, 14.9222],
        icao: "LIRF",
        capabilities: ["Maritime Patrol", "Mediterranean Ops", "ASW"],
        aircraft: ["P-8A Poseidon", "MQ-4C Triton"],
        runway: "05/23 - 2438m",
        frequency: "118.400",
        elevation: "24m"
    },
    "LIRU": {
        name: "Decimomannu Air Base",
        type: "AIR_FORCE",
        country: "Italy",
        coordinates: [39.3547, 8.9728],
        icao: "LIRU",
        capabilities: ["Training", "Mediterranean Patrol"],
        aircraft: ["Eurofighter", "Tornado"],
        runway: "06/24 - 2744m",
        frequency: "122.100",
        elevation: "28m"
    },
    "LIPA": {
        name: "Aviano Air Base",
        type: "AIR_FORCE",
        country: "Italy/USA",
        coordinates: [46.0319, 12.5964],
        icao: "LIPA",
        capabilities: ["NATO Operations", "Training"],
        aircraft: ["F-16", "KC-135"],
        runway: "05/23 - 2900m",
        frequency: "118.600",
        elevation: "129m"
    },
    
    // Spain - NATO Partner
    "LERT": {
        name: "Rota Naval Station",
        type: "NAVAL_AIR",
        country: "Spain/USA",
        coordinates: [36.6453, -6.3489],
        icao: "LERT",
        capabilities: ["Maritime Patrol", "Atlantic Ops", "Refueling"],
        aircraft: ["P-8A Poseidon", "KC-135"],
        runway: "03/21 - 3353m",
        frequency: "118.775",
        elevation: "26m"
    },
    "LERS": {
        name: "Morón Air Base",
        type: "AIR_FORCE",
        country: "Spain/USA",
        coordinates: [37.1742, -5.6158],
        icao: "LERS",
        capabilities: ["Strategic Airlift", "Refueling"],
        aircraft: ["KC-135", "C-130J"],
        runway: "06/24 - 3810m",
        frequency: "121.900",
        elevation: "87m"
    },
    "LEAS": {
        name: "Zaragoza Air Base",
        type: "AIR_FORCE",
        country: "Spain",
        coordinates: [41.6661, -1.0419],
        icao: "LEAS",
        capabilities: ["Training", "Transport"],
        aircraft: ["CN-235", "C-130 Hercules"],
        runway: "12/30 - 3500m",
        frequency: "118.100",
        elevation: "263m"
    },
    
    // Portugal - NATO Partner
    "LPLA": {
        name: "Lajes Field",
        type: "AIR_FORCE",
        country: "Portugal/USA",
        coordinates: [38.7619, -27.0908],
        icao: "LPLA",
        capabilities: ["Maritime Patrol", "Atlantic Operations", "Refueling"],
        aircraft: ["P-8A Poseidon", "KC-10"],
        runway: "15/33 - 3267m",
        frequency: "121.900",
        elevation: "55m"
    },
    "LPMT": {
        name: "Montijo Air Base",
        type: "AIR_FORCE",
        country: "Portugal",
        coordinates: [38.7042, -8.9753],
        icao: "LPMT",
        capabilities: ["Maritime Patrol", "SAR", "Training"],
        aircraft: ["P-3C Orion", "C-130"],
        runway: "04/22 - 2400m",
        frequency: "118.900",
        elevation: "15m"
    },
    
    // Netherlands - NATO Core
    "EHVK": {
        name: "Volkel Air Base",
        type: "AIR_FORCE",
        country: "Netherlands",
        coordinates: [51.6536, 5.7081],
        icao: "EHVK",
        capabilities: ["NATO Operations", "Training"],
        aircraft: ["F-35A", "F-16"],
        runway: "06/24 - 3000m",
        frequency: "119.375",
        elevation: "22m"
    },
    "EHLW": {
        name: "Leeuwarden Air Base",
        type: "AIR_FORCE",
        country: "Netherlands",
        coordinates: [53.2281, 5.7606],
        icao: "EHLW",
        capabilities: ["NATO QRA", "Training"],
        aircraft: ["F-35A", "F-16"],
        runway: "06/24 - 3048m",
        frequency: "121.950",
        elevation: "1m"
    },
    "EHGR": {
        name: "Gilze-Rijen Air Base",
        type: "AIR_FORCE",
        country: "Netherlands",
        coordinates: [51.5678, 4.9319],
        icao: "EHGR",
        capabilities: ["Transport", "Training", "Special Ops"],
        aircraft: ["C-130J", "CH-47 Chinook"],
        runway: "10/28 - 3000m",
        frequency: "118.875",
        elevation: "15m"
    },
    
    // Belgium - NATO Core
    "EBLE": {
        name: "Kleine Brogel Air Base",
        type: "AIR_FORCE",
        country: "Belgium",
        coordinates: [51.1683, 5.4700],
        icao: "EBLE",
        capabilities: ["NATO Operations", "Nuclear Capability"],
        aircraft: ["F-16", "F-35A"],
        runway: "06/24 - 3200m",
        frequency: "122.350",
        elevation: "61m"
    },
    "EBFS": {
        name: "Florennes Air Base",
        type: "AIR_FORCE",
        country: "Belgium",
        coordinates: [50.2433, 4.6456],
        icao: "EBFS",
        capabilities: ["NATO QRA", "Training"],
        aircraft: ["F-16"],
        runway: "06/24 - 3650m",
        frequency: "118.300",
        elevation: "287m"
    },
    
    // Denmark - NATO Partner
    "EKRK": {
        name: "Karup Air Base",
        type: "AIR_FORCE",
        country: "Denmark",
        coordinates: [56.2975, 9.1244],
        icao: "EKRK",
        capabilities: ["Maritime Patrol", "Baltic Ops", "QRA"],
        aircraft: ["F-16", "C-130J"],
        runway: "09/27 - 2700m",
        frequency: "123.300",
        elevation: "53m"
    },
    "EKSP": {
        name: "Skrydstrup Air Base",
        type: "AIR_FORCE",
        country: "Denmark",
        coordinates: [55.2256, 9.2639],
        icao: "EKSP",
        capabilities: ["NATO QRA", "Training"],
        aircraft: ["F-16", "F-35A"],
        runway: "08/26 - 2700m",
        frequency: "118.800",
        elevation: "43m"
    },
    
    // Norway - NATO Partner
    "ENAL": {
        name: "Andøya Air Station",
        type: "AIR_FORCE",
        country: "Norway",
        coordinates: [69.2925, 16.1439],
        icao: "ENAL",
        capabilities: ["Arctic Maritime Patrol", "SIGINT", "Space Tracking"],
        aircraft: ["P-3C Orion", "Falcon 20"],
        runway: "10/28 - 2999m",
        frequency: "119.100",
        elevation: "14m"
    },
    "ENBO": {
        name: "Bodø Air Station",
        type: "AIR_FORCE",
        country: "Norway",
        coordinates: [67.2692, 14.3653],
        icao: "ENBO",
        capabilities: ["Arctic Ops", "NATO QRA"],
        aircraft: ["F-35A", "F-16"],
        runway: "07/25 - 2794m",
        frequency: "118.100",
        elevation: "12m"
    },
    "ENTC": {
        name: "Værnes Air Station",
        type: "AIR_FORCE",
        country: "Norway",
        coordinates: [63.4575, 10.9239],
        icao: "ENTC",
        capabilities: ["Training", "Transport"],
        aircraft: ["C-130J", "Bell 412"],
        runway: "09/27 - 2999m",
        frequency: "118.200",
        elevation: "17m"
    },
    
    // Sweden - NATO Partner
    "ESKN": {
        name: "Såtenäs Air Base",
        type: "AIR_FORCE",
        country: "Sweden",
        coordinates: [58.4364, 12.7144],
        icao: "ESKN",
        capabilities: ["Baltic Maritime Patrol", "Air Defense", "Training"],
        aircraft: ["JAS 39 Gripen"],
        runway: "03/21 - 2500m",
        frequency: "123.050",
        elevation: "58m"
    },
    "ESSB": {
        name: "Malmen Air Base",
        type: "AIR_FORCE",
        country: "Sweden",
        coordinates: [58.4022, 15.5256],
        icao: "ESSB",
        capabilities: ["Training", "Support"],
        aircraft: ["JAS 39 Gripen", "Tp 84"],
        runway: "03/21 - 2440m",
        frequency: "122.300",
        elevation: "93m"
    },
    
    // Finland - NATO Partner
    "EFHK": {
        name: "Helsinki-Malmi Airport",
        type: "AIR_FORCE",
        country: "Finland",
        coordinates: [60.2542, 25.0428],
        icao: "EFHK",
        capabilities: ["Baltic Maritime Patrol", "Border Surveillance"],
        aircraft: ["F/A-18 Hornet"],
        runway: "09/27 - 1000m",
        frequency: "122.500",
        elevation: "17m"
    },
    "EFRV": {
        name: "Rovaniemi Airport",
        type: "AIR_FORCE",
        country: "Finland",
        coordinates: [66.5647, 25.8303],
        icao: "EFRV",
        capabilities: ["Arctic Ops", "Training"],
        aircraft: ["F/A-18 Hornet"],
        runway: "03/21 - 3000m",
        frequency: "119.800",
        elevation: "198m"
    },
    
    // Iceland - NATO Partner
    "BIKF": {
        name: "Keflavik Air Base",
        type: "AIR_FORCE",
        country: "Iceland/NATO",
        coordinates: [63.9850, -22.6056],
        icao: "BIKF",
        capabilities: ["GIUK Gap Patrol", "Atlantic Ops", "NATO QRA"],
        aircraft: ["F-35A", "C-130"],
        runway: "11/29 - 3065m",
        frequency: "118.900",
        elevation: "56m"
    },
    
    // Turkey - NATO Partner
    "LTAD": {
        name: "Incirlik Air Base",
        type: "AIR_FORCE",
        country: "Turkey/USA",
        coordinates: [37.2392, 35.4258],
        icao: "LTAD",
        capabilities: ["Mediterranean Ops", "Middle East Operations"],
        aircraft: ["F-16", "KC-135"],
        runway: "05/23 - 3048m",
        frequency: "122.100",
        elevation: "69m"
    },
    "LTBG": {
        name: "Dalaman Air Base",
        type: "AIR_FORCE",
        country: "Turkey",
        coordinates: [36.7131, 28.7925],
        icao: "LTBG",
        capabilities: ["Mediterranean Patrol", "Training"],
        aircraft: ["F-16", "CN-235"],
        runway: "03/21 - 3300m",
        frequency: "119.100",
        elevation: "8m"
    },
    
    // Greece - NATO Partner
    "LGZA": {
        name: "Andravida Air Base",
        type: "AIR_FORCE",
        country: "Greece",
        coordinates: [37.9203, 21.2925],
        icao: "LGZA",
        capabilities: ["Maritime Patrol", "Ionian Sea Ops"],
        aircraft: ["F-16", "C-130"],
        runway: "17/35 - 3500m",
        frequency: "120.100",
        elevation: "14m"
    },
    "LGSK": {
        name: "Skaramanga Naval Base",
        type: "NAVAL_AIR",
        country: "Greece",
        coordinates: [37.9628, 23.5631],
        icao: "LGSK",
        capabilities: ["Aegean Sea Patrol", "ASW"],
        aircraft: ["AB 212", "P-3B Orion"],
        runway: "09/27 - 1200m",
        frequency: "118.300",
        elevation: "3m"
    },
    
    // Pacific Region - US/Allied Bases
    "RJTY": {
        name: "Yokota Air Base",
        type: "AIR_FORCE",
        country: "Japan/USA",
        coordinates: [35.7485, 139.3481],
        icao: "RJTY",
        capabilities: ["Pacific Maritime Patrol", "ISR", "Airlift"],
        aircraft: ["C-130J", "CV-22", "C-12"],
        runway: "18/36 - 3353m",
        frequency: "118.200",
        elevation: "142m"
    },
    "RJOI": {
        name: "Iwakuni Marine Corps Air Station",
        type: "MARINE_AIR",
        country: "Japan/USA",
        coordinates: [34.1442, 132.2356],
        icao: "RJOI",
        capabilities: ["Maritime Patrol", "Amphibious Ops", "Fighter Ops"],
        aircraft: ["F-35B", "KC-130J", "MV-22"],
        runway: "02/20 - 2440m",
        frequency: "126.200",
        elevation: "7m"
    },
    "RJTA": {
        name: "Kadena Air Base",
        type: "AIR_FORCE",
        country: "Japan/USA",
        coordinates: [26.3556, 127.7681],
        icao: "RJTA",
        capabilities: ["Pacific Ops", "Strategic Operations"],
        aircraft: ["F-15", "KC-135", "E-3"],
        runway: "05L/23R - 3700m",
        frequency: "126.200",
        elevation: "46m"
    },
    "RJAM": {
        name: "Miho Air Base",
        type: "AIR_FORCE",
        country: "Japan",
        coordinates: [35.4922, 133.2356],
        icao: "RJAM",
        capabilities: ["Training", "Transport"],
        aircraft: ["C-1", "T-4"],
        runway: "07/25 - 2743m",
        frequency: "118.200",
        elevation: "7m"
    },
    "RKSO": {
        name: "Osan Air Base",
        type: "AIR_FORCE",
        country: "South Korea/USA",
        coordinates: [37.0908, 127.0297],
        icao: "RKSO",
        capabilities: ["Maritime Patrol", "Peninsula Defense", "ISR"],
        aircraft: ["F-16", "A-10", "U-2"],
        runway: "09/27 - 2743m",
        frequency: "121.200",
        elevation: "38m"
    },
    "RKJK": {
        name: "Kunsan Air Base",
        type: "AIR_FORCE",
        country: "South Korea/USA",
        coordinates: [35.9036, 126.6158],
        icao: "RKJK",
        capabilities: ["Yellow Sea Ops", "Training"],
        aircraft: ["F-16", "A-10"],
        runway: "18/36 - 2743m",
        frequency: "118.200",
        elevation: "9m"
    },
    
    // Australia - Indo-Pacific Partner
    "YMAV": {
        name: "RAAF Base Avalon",
        type: "AIR_FORCE",
        country: "Australia",
        coordinates: [-38.0394, 144.4689],
        icao: "YMAV",
        capabilities: ["Training", "Test & Evaluation"],
        aircraft: ["F/A-18", "C-130J"],
        runway: "18/36 - 2286m",
        frequency: "120.100",
        elevation: "35m"
    },
    "YPED": {
        name: "RAAF Base Edinburgh",
        type: "AIR_FORCE",
        country: "Australia",
        coordinates: [-34.7056, 138.6206],
        icao: "YPED",
        capabilities: ["Maritime Patrol", "ISR", "Training"],
        aircraft: ["P-8A Poseidon", "AP-3C Orion"],
        runway: "05/23 - 2682m",
        frequency: "118.200",
        elevation: "20m"
    },
    "YBPN": {
        name: "RAAF Base Pearce",
        type: "AIR_FORCE",
        country: "Australia",
        coordinates: [-31.6747, 116.0156],
        icao: "YBPN",
        capabilities: ["Training", "Maritime Support"],
        aircraft: ["Hawk 127", "King Air"],
        runway: "18/36 - 1829m",
        frequency: "118.100",
        elevation: "32m"
    },
    
    // Eastern Europe - New NATO Members
    "EPKS": {
        name: "Kościuszko Air Base",
        type: "AIR_FORCE",
        country: "Poland",
        coordinates: [50.1122, 20.9647],
        icao: "EPKS",
        capabilities: ["NATO Operations", "Training"],
        aircraft: ["F-16", "MiG-29"],
        runway: "07/25 - 2500m",
        frequency: "119.100",
        elevation: "218m"
    },
    "EPMI": {
        name: "Minsk Mazowiecki Air Base",
        type: "AIR_FORCE",
        country: "Poland",
        coordinates: [52.1953, 21.6503],
        icao: "EPMI",
        capabilities: ["NATO QRA", "Training"],
        aircraft: ["F-16", "MiG-29"],
        runway: "08/26 - 2500m",
        frequency: "118.700",
        elevation: "186m"
    },
    "LHKE": {
        name: "Kecskemet Air Base",
        type: "AIR_FORCE",
        country: "Hungary",
        coordinates: [46.9153, 19.7492],
        icao: "LHKE",
        capabilities: ["NATO QRA", "Training"],
        aircraft: ["JAS 39 Gripen"],
        runway: "13/31 - 2500m",
        frequency: "118.500",
        elevation: "120m"
    },
    "LRCK": {
        name: "Campia Turzii Air Base",
        type: "AIR_FORCE",
        country: "Romania",
        coordinates: [46.5031, 23.8686],
        icao: "LRCK",
        capabilities: ["NATO Operations", "Training"],
        aircraft: ["F-16", "IAR 330"],
        runway: "07/25 - 3500m",
        frequency: "118.600",
        elevation: "346m"
    },
    
    // North America
    "KNHK": {
        name: "Patuxent River NAS",
        type: "NAVAL_AIR",
        country: "United States",
        coordinates: [38.2856, -76.4119],
        icao: "KNHK",
        capabilities: ["Maritime Patrol", "Test & Evaluation", "Atlantic Ops"],
        aircraft: ["P-8A Poseidon", "MQ-4C Triton"],
        runway: "06/24 - 3658m",
        frequency: "120.600",
        elevation: "14m"
    },
    "KNIP": {
        name: "Naval Air Station Jacksonville",
        type: "NAVAL_AIR",
        country: "United States",
        coordinates: [30.2358, -81.6806],
        icao: "KNIP",
        capabilities: ["Maritime Patrol", "ASW", "Atlantic Fleet"],
        aircraft: ["P-8A Poseidon", "SH-60"],
        runway: "09/27 - 2438m",
        frequency: "118.500",
        elevation: "8m"
    },
    "PHNL": {
        name: "Hickam Air Force Base",
        type: "AIR_FORCE",
        country: "United States",
        coordinates: [21.3186, -157.9225],
        icao: "PHNL",
        capabilities: ["Pacific Maritime Patrol", "Strategic Ops", "Airlift"],
        aircraft: ["KC-135", "C-17", "F-22"],
        runway: "08L/26R - 3749m",
        frequency: "118.100",
        elevation: "4m"
    },
    "CYYG": {
        name: "CFB Comox",
        type: "AIR_FORCE",
        country: "Canada",
        coordinates: [49.7108, -124.8867],
        icao: "CYYG",
        capabilities: ["Maritime Patrol", "SAR", "Pacific Ops"],
        aircraft: ["CP-140 Aurora", "CC-115 Buffalo"],
        runway: "11/29 - 3048m",
        frequency: "119.700",
        elevation: "21m"
    },
    
    // Northern Europe / Arctic
    "ENAL": {
        name: "Andøya Air Station",
        type: "AIR_FORCE",
        country: "Norway",
        coordinates: [69.2925, 16.1439],
        icao: "ENAL",
        capabilities: ["Arctic Maritime Patrol", "SIGINT", "Space Tracking"],
        aircraft: ["P-3C Orion", "Falcon 20"],
        runway: "10/28 - 2999m",
        frequency: "119.100",
        elevation: "14m"
    },
    "EFHK": {
        name: "Helsinki-Malmi Airport",
        type: "AIR_FORCE",
        country: "Finland",
        coordinates: [60.2542, 25.0428],
        icao: "EFHK",
        capabilities: ["Baltic Maritime Patrol", "Border Surveillance"],
        aircraft: ["F/A-18 Hornet"],
        runway: "09/27 - 1000m",
        frequency: "122.500",
        elevation: "17m"
    },
    "ESKN": {
        name: "Såtenäs Air Base",
        type: "AIR_FORCE",
        country: "Sweden",
        coordinates: [58.4364, 12.7144],
        icao: "ESKN",
        capabilities: ["Baltic Maritime Patrol", "Air Defense", "Training"],
        aircraft: ["JAS 39 Gripen"],
        runway: "03/21 - 2500m",
        frequency: "123.050",
        elevation: "58m"
    },
    
    // Naval Bases
    "NAVAL_NORFOLK": {
        name: "Naval Station Norfolk",
        type: "NAVAL_BASE",
        country: "United States",
        coordinates: [36.9147, -76.2947],
        icao: "N/A",
        capabilities: ["Fleet Operations", "Atlantic Command", "Carrier Ops"],
        ships: ["CVN", "DDG", "CG", "LHD"],
        frequency: "143.625",
        elevation: "3m"
    },
    "NAVAL_DEVONPORT": {
        name: "HMNB Devonport",
        type: "NAVAL_BASE",
        country: "United Kingdom", 
        coordinates: [50.3833, -4.1833],
        icao: "N/A",
        capabilities: ["Royal Navy Operations", "Submarine Base", "Atlantic Ops"],
        ships: ["Type 45", "Type 23", "Astute Class"],
        frequency: "156.800",
        elevation: "15m"
    },
    "NAVAL_PEARL": {
        name: "Pearl Harbor Naval Base",
        type: "NAVAL_BASE",
        country: "United States",
        coordinates: [21.3647, -157.9564],
        icao: "N/A",
        capabilities: ["Pacific Fleet", "Submarine Ops", "Strategic Command"],
        ships: ["DDG", "SSN", "LCS"],
        frequency: "142.450",
        elevation: "3m"
    },
    "NAVAL_YOKOSUKA": {
        name: "Naval Base Yokosuka",
        type: "NAVAL_BASE",
        country: "Japan/USA",
        coordinates: [35.2889, 139.6672],
        icao: "N/A",
        capabilities: ["Forward Deployed Naval Forces", "Carrier Ops", "Pacific Ops"],
        ships: ["CVN", "DDG", "CG"],
        frequency: "141.775",
        elevation: "5m"
    }
};

// Mission-specific patrol areas based on base location
const PATROL_AREAS = {
    "EGUN": ["North Sea", "English Channel", "Celtic Sea"],
    "EGXN": ["North Atlantic", "Norwegian Sea", "Shetland Gap"],
    "LPLA": ["Mid-Atlantic", "Azores Triangle", "Gibraltar Strait"],
    "LIRF": ["Central Mediterranean", "Sicily Channel", "Ionian Sea"],
    "EKRK": ["Baltic Sea", "Kattegat", "Skagerrak"],
    "RJTY": ["Sea of Japan", "East China Sea", "Philippine Sea"],
    "RJOI": ["East China Sea", "Yellow Sea", "Korea Strait"],
    "RKSO": ["Yellow Sea", "Korea Bay", "Tsushima Strait"],
    "WMKK": ["Strait of Malacca", "South China Sea", "Andaman Sea"],
    "KNHK": ["Chesapeake Bay", "Atlantic Coast", "Caribbean"],
    "KNIP": ["Gulf of Mexico", "Florida Straits", "Bahamas"],
    "PHNL": ["Central Pacific", "Hawaiian Islands", "Aleutian Chain"],
    "CYYG": ["Strait of Georgia", "Juan de Fuca", "Queen Charlotte"],
    "ENAL": ["Norwegian Sea", "Barents Sea", "Greenland Sea"],
    "EFHK": ["Gulf of Finland", "Baltic Sea", "Gulf of Bothnia"],
    "ESKN": ["Baltic Sea", "Kattegat", "North Sea"]
};

// Available aircraft by base type and mission
const BASE_AIRCRAFT = {
    "AIR_FORCE": {
        "PRIMARY": ["P-8A Poseidon", "P-3C Orion", "CP-140 Aurora"],
        "SECONDARY": ["C-130J Hercules", "KC-135 Stratotanker", "F-16 Fighting Falcon"],
        "SPECIAL": ["U-2 Dragon Lady", "RC-135 Rivet Joint", "E-3 AWACS"]
    },
    "NAVAL_AIR": {
        "PRIMARY": ["P-8A Poseidon", "MQ-4C Triton", "SH-60 Seahawk"],
        "SECONDARY": ["C-2 Greyhound", "E-2 Hawkeye", "MH-53E Sea Dragon"],
        "SPECIAL": ["EP-3E Aries", "MQ-9 Reaper"]
    },
    "MARINE_AIR": {
        "PRIMARY": ["F-35B Lightning II", "AV-8B Harrier", "MV-22 Osprey"],
        "SECONDARY": ["KC-130J Super Hercules", "UH-1Y Venom", "AH-1Z Viper"],
        "SPECIAL": ["EA-6B Prowler"]
    },
    "NAVAL_BASE": {
        "PRIMARY": ["Ship-based Operations"],
        "SECONDARY": ["Port Security", "Coastal Patrol"],
        "SPECIAL": ["Fleet Support"]
    }
};

// Threat assessment by region
const REGIONAL_THREATS = {
    "North Sea": { level: "LOW", description: "Commercial shipping, weather" },
    "Mediterranean": { level: "MEDIUM", description: "Migration routes, smuggling" },
    "Baltic Sea": { level: "MEDIUM", description: "Regional tensions, submarine activity" },
    "Pacific": { level: "HIGH", description: "Strategic competition, territorial disputes" },
    "South China Sea": { level: "HIGH", description: "Maritime claims, military buildup" },
    "Atlantic": { level: "LOW", description: "Standard maritime traffic" },
    "Arctic": { level: "MEDIUM", description: "Resource competition, environmental" }
};

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = {
        TACTICAL_BASES,
        PATROL_AREAS,
        BASE_AIRCRAFT,
        REGIONAL_THREATS
    };
}