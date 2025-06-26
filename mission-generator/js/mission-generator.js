// Mission Generation Logic
class MissionGenerator {
    constructor() {
        this.missionTypes = {
            'MARITIME_PATROL': {
                name: 'Maritime Patrol',
                description: 'Search and surveillance of designated maritime areas',
                objectives: [
                    'Conduct surface surveillance of assigned patrol area',
                    'Report any suspicious vessel activity to Maritime Operations Center',
                    'Use FLIR camera system for vessel identification and classification',
                    'Maintain communication with home base every 30 minutes',
                    'Document all surface contacts with position and bearing'
                ],
                duration: '4-8 hours',
                minAltitude: 1000,
                maxAltitude: 25000,
                equipment: ['FLIR-25HD Camera', 'Maritime Radar', 'ESM Suite', 'SATCOM']
            },
            'SAR': {
                name: 'Search and Rescue',
                description: 'Locate and assist vessels or aircraft in distress',
                objectives: [
                    'Search designated area using FLIR and visual systems',
                    'Identify and classify any surface contacts or debris',
                    'Coordinate with rescue vessels and helicopters if targets located',
                    'Provide accurate position updates to SAR coordination center',
                    'Maintain search pattern integrity throughout operation'
                ],
                duration: '6-10 hours',
                minAltitude: 500,
                maxAltitude: 15000,
                equipment: ['FLIR-25HD Camera', 'SAR Radar', 'Emergency Beacons', 'Life Rafts']
            },
            'RECON': {
                name: 'Reconnaissance',
                description: 'Intelligence gathering over specified maritime areas',
                objectives: [
                    'Conduct covert surveillance of designated target area',
                    'Use FLIR camera system for detailed observation and recording',
                    'Document vessel movements, types, and activities',
                    'Maintain operational security throughout mission',
                    'Collect electronic intelligence where possible'
                ],
                duration: '3-6 hours',
                minAltitude: 2000,
                maxAltitude: 30000,
                equipment: ['FLIR-25HD Camera', 'Electronic Intelligence Suite', 'Secure Communications']
            },
            'ASW': {
                name: 'Anti-Submarine Warfare',
                description: 'Submarine detection and tracking operations',
                objectives: [
                    'Deploy sonobuoys in designated search pattern',
                    'Monitor passive and active sonar contacts',
                    'Use FLIR to identify surface vessels and periscopes',
                    'Coordinate with naval vessels for prosecution',
                    'Maintain acoustic tracking of subsurface contacts'
                ],
                duration: '6-12 hours',
                minAltitude: 200,
                maxAltitude: 10000,
                equipment: ['FLIR-25HD Camera', 'Sonobuoy Dispenser', 'MAD Boom', 'Torpedoes']
            },
            'FISHERY_PATROL': {
                name: 'Fishery Protection',
                description: 'Monitor fishing activities and enforce maritime regulations',
                objectives: [
                    'Patrol designated fishing zones and protected areas',
                    'Identify and inspect fishing vessels using FLIR camera',
                    'Report illegal fishing activities to coast guard',
                    'Document vessel positions and fishing equipment',
                    'Coordinate with patrol vessels for enforcement action'
                ],
                duration: '4-8 hours',
                minAltitude: 1000,
                maxAltitude: 15000,
                equipment: ['FLIR-25HD Camera', 'Maritime Radar', 'Camera Systems', 'GPS Tracker']
            }
        };
    }

    generateMission(airbaseIcao, missionType, patrolArea) {
        const airbase = MILITARY_AIRBASES[airbaseIcao];
        const mission = this.missionTypes[missionType];
        const area = PATROL_AREAS[patrolArea];
        
        if (!airbase || !mission || !area) {
            throw new Error('Invalid mission parameters');
        }

        const missionData = {
            id: this.generateMissionId(),
            codename: this.generateCodename(),
            type: mission,
            airbase: airbase,
            airbaseIcao: airbaseIcao,
            patrolArea: area,
            patrolAreaKey: patrolArea,
            callsign: this.generateCallsign(airbase.country),
            aircraft: this.selectAircraft(airbase.aircraft, missionType),
            
            // Timing
            startTime: this.generateMissionTime(),
            duration: mission.duration,
            
            // Navigation
            waypoints: generatePatrolWaypoints(patrolArea, 4),
            distance: calculateDistanceToPatrolArea(airbase.coords, patrolArea),
            
            // Communications
            frequencies: this.generateFrequencies(),
            
            // Security
            authCode: this.generateAuthCode(),
            classification: 'NATO RESTRICTED',
            
            // Environmental
            threatLevel: this.generateThreatLevel(),
            
            // Generated timestamp
            generatedAt: new Date().toISOString()
        };

        return missionData;
    }

    generateMissionId() {
        const prefix = 'MP'; // Maritime Patrol
        const date = new Date().toISOString().slice(2, 10).replace(/-/g, '');
        const random = Math.floor(Math.random() * 1000).toString().padStart(3, '0');
        return `${prefix}${date}${random}`;
    }

    generateCodename() {
        const adjectives = [
            'NORTHERN', 'SOUTHERN', 'EASTERN', 'WESTERN', 'CENTRAL',
            'BLUE', 'GREEN', 'RED', 'GOLD', 'SILVER',
            'SWIFT', 'SILENT', 'DEEP', 'HIGH', 'LONG',
            'SHARP', 'BRIGHT', 'DARK', 'CLEAR', 'STORM'
        ];
        
        const nouns = [
            'SENTINEL', 'GUARDIAN', 'WATCHER', 'HUNTER', 'SEEKER',
            'FALCON', 'EAGLE', 'HAWK', 'RAVEN', 'ALBATROSS',
            'TRIDENT', 'ANCHOR', 'COMPASS', 'BEACON', 'LIGHTHOUSE',
            'WAVE', 'TIDE', 'CURRENT', 'REEF', 'DEPTH'
        ];

        const adj = adjectives[Math.floor(Math.random() * adjectives.length)];
        const noun = nouns[Math.floor(Math.random() * nouns.length)];
        
        return `${adj} ${noun}`;
    }

    generateCallsign(country) {
        const callsigns = {
            'USA': ['POSEIDON', 'CLIPPER', 'SEAHAWK', 'NEPTUNE', 'MARINER'],
            'UK': ['KINGFISHER', 'NIMROD', 'GUARDIAN', 'SENTINEL', 'PHOENIX'],
            'France': ['ATLANTIQUE', 'FALCON', 'DAUPHIN', 'MISTRAL', 'NAVAL'],
            'Germany': ['ORION', 'SEAKING', 'HURRICANE', 'VIKING', 'MARITIME'],
            'Italy': ['ATLANTICO', 'SPARTAN', 'HARRIER', 'VESUVIO', 'MARE'],
            'Canada': ['AURORA', 'MAPLE', 'ARCTIC', 'PACIFIC', 'ATLANTIC'],
            'Australia': ['WEDGETAIL', 'SOUTHERN', 'PACIFIC', 'ANZAC', 'CORAL'],
            'Netherlands': ['ORANGE', 'FALCON', 'SEAHORSE', 'NORDKAPP', 'ZUIDERZEE'],
            'Norway': ['POLAR', 'VIKING', 'ARCTIC', 'FJORD', 'MIDNIGHT'],
            'Spain': ['EAGLE', 'IBERIAN', 'ATLANTIC', 'PELICAN', 'GIBRALTAR'],
            'Portugal': ['NAVIGATOR', 'ATLANTIC', 'AZORES', 'CORMORANT', 'MAGELLAN']
        };

        const countryCallsigns = callsigns[country] || callsigns['USA'];
        const base = countryCallsigns[Math.floor(Math.random() * countryCallsigns.length)];
        const number = Math.floor(Math.random() * 99) + 1;
        
        return `${base} ${number.toString().padStart(2, '0')}`;
    }

    selectAircraft(availableAircraft, missionType) {
        // Mission-specific aircraft preferences
        const preferences = {
            'MARITIME_PATROL': ['P-8A Poseidon', 'P-3C Orion', 'Atlantique 2', 'CP-140 Aurora'],
            'SAR': ['P-8A Poseidon', 'P-3C Orion', 'C-130J Super Hercules', 'ATR 72MP'],
            'RECON': ['P-8A Poseidon', 'EP-3E Aries', 'Atlantique 2', 'ATR 72MP'],
            'ASW': ['P-8A Poseidon', 'P-3C Orion', 'Atlantique 2', 'CP-140 Aurora'],
            'FISHERY_PATROL': ['P-3C Orion', 'ATR 72MP', 'C-295M', 'Falcon 50M']
        };

        const preferred = preferences[missionType] || availableAircraft;
        
        // Find best match
        for (const aircraft of preferred) {
            if (availableAircraft.includes(aircraft)) {
                return aircraft;
            }
        }
        
        // Fallback to first available
        return availableAircraft[0] || 'P-8A Poseidon';
    }

    generateMissionTime() {
        const now = new Date();
        // Generate mission start time 1-4 hours from now
        const hoursFromNow = Math.floor(Math.random() * 3) + 1;
        const startTime = new Date(now.getTime() + hoursFromNow * 60 * 60 * 1000);
        
        // Round to nearest 15 minutes
        const minutes = Math.round(startTime.getMinutes() / 15) * 15;
        startTime.setMinutes(minutes, 0, 0);
        
        return startTime.toISOString().slice(11, 16) + ' ZULU';
    }

    generateFrequencies() {
        return {
            base: `${(250 + Math.random() * 150).toFixed(3)}`,
            maritime: `${(250 + Math.random() * 150).toFixed(3)}`,
            guard: '121.500',
            emergency: '243.000',
            sar: `${(250 + Math.random() * 150).toFixed(3)}`
        };
    }

    generateAuthCode() {
        const words = [
            'ALPHA', 'BRAVO', 'CHARLIE', 'DELTA', 'ECHO', 'FOXTROT',
            'GOLF', 'HOTEL', 'INDIA', 'JULIET', 'KILO', 'LIMA',
            'MIKE', 'NOVEMBER', 'OSCAR', 'PAPA', 'QUEBEC', 'ROMEO',
            'SIERRA', 'TANGO', 'UNIFORM', 'VICTOR', 'WHISKEY', 'XRAY',
            'YANKEE', 'ZULU'
        ];
        
        const word1 = words[Math.floor(Math.random() * words.length)];
        const word2 = words[Math.floor(Math.random() * words.length)];
        const number = Math.floor(Math.random() * 100).toString().padStart(2, '0');
        
        return `${word1}-${word2}-${number}`;
    }

    generateThreatLevel() {
        const levels = ['LOW', 'MODERATE', 'ELEVATED', 'HIGH'];
        const weights = [0.4, 0.35, 0.2, 0.05]; // Probability weights
        
        const random = Math.random();
        let cumulative = 0;
        
        for (let i = 0; i < levels.length; i++) {
            cumulative += weights[i];
            if (random <= cumulative) {
                return levels[i];
            }
        }
        
        return 'LOW';
    }

    generateBriefing(missionData) {
        const { 
            codename, type, callsign, aircraft, airbase, airbaseIcao,
            patrolArea, startTime, duration, waypoints, distance,
            frequencies, authCode, classification, threatLevel
        } = missionData;

        return `═══════════════════════════════════════
         MARITIME PATROL MISSION
            OPERATION ${codename}
═══════════════════════════════════════

CLASSIFICATION: ${classification}

MISSION TYPE: ${type.name}
CALLSIGN: ${callsign}
AIRCRAFT: ${aircraft}

DEPARTURE: ${airbase.name} (${airbaseIcao})
PATROL AREA: ${patrolArea.name}
MISSION START: ${startTime}
ESTIMATED DURATION: ${duration}
DISTANCE TO AREA: ${distance} nm

PRIMARY OBJECTIVES:
${type.objectives.map(obj => `▶ ${obj}`).join('\n')}

AREA OF OPERATIONS:
${patrolArea.description}

KEY PATROL AREAS:
${patrolArea.keyAreas ? patrolArea.keyAreas.map(area => `• ${area}`).join('\n') : 'Standard patrol pattern'}

PATROL WAYPOINTS:
${waypoints.map((wp, i) => `${i + 1}. ${wp.name}: ${wp.coords[0].toFixed(3)}°N ${Math.abs(wp.coords[1]).toFixed(3)}°${wp.coords[1] < 0 ? 'W' : 'E'}`).join('\n')}

EQUIPMENT LOADOUT:
${type.equipment.map(eq => `▶ ${eq}`).join('\n')}

FLIR CAMERA OPERATION:
▶ F9: Activate/Deactivate Camera
▶ Mouse: Pan/Tilt Control  
▶ +/-: Zoom In/Out
▶ Arrow Keys: Fine Adjustment
▶ T: Cycle Visual Modes (Standard/Mono/Thermal)
▶ SPACE: Target Lock (Red=Locked, Green=Scanning)

COMMUNICATION FREQUENCIES:
Home Base: ${frequencies.base}
Maritime Ops: ${frequencies.maritime}
SAR Coordination: ${frequencies.sar}
Guard: ${frequencies.guard}
Emergency: ${frequencies.emergency}

THREAT ASSESSMENT: ${threatLevel}
${this.getThreatDescription(threatLevel)}

POTENTIAL HAZARDS:
${patrolArea.threats ? patrolArea.threats.map(threat => `⚠ ${threat}`).join('\n') : '⚠ Standard maritime hazards'}

SPECIAL INSTRUCTIONS:
▶ Report all surface contacts immediately
▶ Maintain minimum altitude ${type.minAltitude}ft AGL
▶ Weather updates every 30 minutes
▶ Emergency procedures per SOP-MAR-001

AUTHENTICATION CODE: ${authCode}

MISSION COMMANDER: Duty Operations Officer
WEATHER BRIEFING: Available via SimBrief/PFPX
NOTAMS: Check current NOTAMs for patrol area

═══════════════════════════════════════
    GOOD HUNTING - STAY VIGILANT
═══════════════════════════════════════

Generated: ${new Date().toUTCString()}`;
    }

    getThreatDescription(level) {
        const descriptions = {
            'LOW': 'Routine patrol environment. Standard precautions apply.',
            'MODERATE': 'Heightened awareness required. Additional reporting protocols in effect.',
            'ELEVATED': 'Increased security measures. Avoid unnecessary risks.',
            'HIGH': 'Significant threat indicators. Mission-critical security protocols active.'
        };
        return descriptions[level] || descriptions['LOW'];
    }
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { MissionGenerator };
}