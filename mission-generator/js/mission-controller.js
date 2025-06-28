// Mission Controller
// Handles mission generation and briefing creation

class MissionController {
    constructor() {
        this.currentMission = null;
        this.selectedBase = null;
        this.operationCounter = 1;
    }

    // Generate a new tactical mission based on parameters
    generateMission(baseId, operationType, threatLevel, duration) {
        const base = TACTICAL_BASES[baseId];
        if (!base) return null;

        this.selectedBase = base;
        
        // Generate mission callsign
        const missionCallsign = this.generateCallsign(base.type);
        
        // Select appropriate aircraft
        const aircraft = this.selectAircraft(base, operationType);
        
        // Generate patrol area
        const patrolArea = this.selectPatrolArea(baseId, operationType);
        
        // Create mission objectives
        const objectives = this.generateObjectives(operationType, threatLevel, patrolArea);
        
        // Generate frequencies and codes
        const comms = this.generateCommunications();
        
        this.currentMission = {
            id: `MPOC-${String(this.operationCounter).padStart(3, '0')}`,
            callsign: missionCallsign,
            base: base,
            operationType: operationType,
            threatLevel: threatLevel,
            duration: duration,
            aircraft: aircraft,
            patrolArea: patrolArea,
            objectives: objectives,
            communications: comms,
            weather: this.generateWeather(),
            timing: this.generateTiming(),
            created: new Date()
        };

        this.operationCounter++;
        return this.currentMission;
    }

    // Generate military-style callsign
    generateCallsign(baseType) {
        const prefixes = {
            'AIR_FORCE': ['EAGLE', 'FALCON', 'HAWK', 'VIPER', 'THUNDER'],
            'NAVAL_AIR': ['TRIDENT', 'POSEIDON', 'NEPTUNE', 'ANCHOR', 'WAVE'],
            'MARINE_AIR': ['DEVIL', 'WARRIOR', 'COBRA', 'STORM', 'LIGHTNING'],
            'NAVAL_BASE': ['FLEET', 'HARBOR', 'TIDE', 'CURRENT', 'DEPTH']
        };
        
        const prefix = prefixes[baseType] || prefixes['AIR_FORCE'];
        const selectedPrefix = prefix[Math.floor(Math.random() * prefix.length)];
        const number = Math.floor(Math.random() * 99) + 1;
        
        return `${selectedPrefix} ${String(number).padStart(2, '0')}`;
    }

    // Select appropriate aircraft for mission
    selectAircraft(base, operationType) {
        const aircraftDb = BASE_AIRCRAFT[base.type] || BASE_AIRCRAFT['AIR_FORCE'];
        
        let category = 'PRIMARY';
        if (operationType === 'RECON' || operationType === 'ASW') {
            category = 'SPECIAL';
        }
        
        const availableAircraft = aircraftDb[category];
        const selected = availableAircraft[Math.floor(Math.random() * availableAircraft.length)];
        
        return {
            type: selected,
            tail: this.generateTailNumber(),
            crew: this.generateCrewSize(selected),
            fuel: this.calculateFuelLoad(selected),
            weapons: this.selectWeapons(selected, operationType)
        };
    }

    // Generate aircraft tail number
    generateTailNumber() {
        const letters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
        const numbers = '0123456789';
        
        return Array.from({length: 2}, () => letters[Math.floor(Math.random() * letters.length)]).join('') +
               '-' +
               Array.from({length: 3}, () => numbers[Math.floor(Math.random() * numbers.length)]).join('');
    }

    // Generate crew size based on aircraft
    generateCrewSize(aircraft) {
        const crewSizes = {
            'P-8A Poseidon': 9,
            'P-3C Orion': 11,
            'CP-140 Aurora': 10,
            'F-16 Fighting Falcon': 1,
            'F-35B Lightning II': 1,
            'C-130J Hercules': 4,
            'MQ-4C Triton': 0, // Unmanned
            'Default': 2
        };
        
        return crewSizes[aircraft] || crewSizes['Default'];
    }

    // Calculate fuel load
    calculateFuelLoad(aircraft) {
        const fuelCapacities = {
            'P-8A Poseidon': 34000,
            'P-3C Orion': 18000,
            'F-16 Fighting Falcon': 3200,
            'F-35B Lightning II': 6100,
            'C-130J Hercules': 19000,
            'Default': 8000
        };
        
        const maxFuel = fuelCapacities[aircraft] || fuelCapacities['Default'];
        const loadFactor = 0.75 + (Math.random() * 0.2); // 75-95% fuel load
        
        return Math.round(maxFuel * loadFactor);
    }

    // Select weapons loadout
    selectWeapons(aircraft, operationType) {
        const weaponLoadouts = {
            'P-8A Poseidon': {
                'SURVEILLANCE': ['Sonobuoys', 'Camera Systems'],
                'ASW': ['Mk 54 Torpedoes', 'Sonobuoys', 'Harpoon Missiles'],
                'INTERDICTION': ['Harpoon Missiles', 'Mk 82 Bombs'],
                'Default': ['Standard Sensors']
            },
            'F-16 Fighting Falcon': {
                'PATROL': ['AIM-120 AMRAAM', 'AIM-9 Sidewinder'],
                'INTERDICTION': ['AGM-88 HARM', 'Mk 82 Bombs'],
                'Default': ['20mm Cannon', 'AIM-120 AMRAAM']
            },
            'Default': {
                'Default': ['Standard Equipment']
            }
        };
        
        const aircraftWeapons = weaponLoadouts[aircraft] || weaponLoadouts['Default'];
        return aircraftWeapons[operationType] || aircraftWeapons['Default'];
    }

    // Select patrol area based on base location
    selectPatrolArea(baseId, operationType) {
        const areas = PATROL_AREAS[baseId] || ['Coastal Waters'];
        const selectedArea = areas[Math.floor(Math.random() * areas.length)];
        
        return {
            name: selectedArea,
            coordinates: this.generatePatrolCoordinates(baseId),
            radius: this.calculatePatrolRadius(operationType),
            altitude: this.selectPatrolAltitude(operationType)
        };
    }

    // Generate patrol area coordinates
    generatePatrolCoordinates(baseId) {
        const base = TACTICAL_BASES[baseId];
        if (!base) return [0, 0];
        
        // Generate coordinates 50-200 nm from base
        const distance = (Math.random() * 150 + 50) / 60; // Convert nm to degrees
        const bearing = Math.random() * 360;
        
        const lat = base.coordinates[0] + (distance * Math.cos(bearing * Math.PI / 180));
        const lon = base.coordinates[1] + (distance * Math.sin(bearing * Math.PI / 180));
        
        return [lat, lon];
    }

    // Calculate patrol radius
    calculatePatrolRadius(operationType) {
        const radii = {
            'SURVEILLANCE': 25,
            'SAR': 50,
            'INTERDICTION': 15,
            'ASW': 30,
            'PATROL': 40,
            'RECON': 20
        };
        
        return radii[operationType] || 30;
    }

    // Select patrol altitude
    selectPatrolAltitude(operationType) {
        const altitudes = {
            'SURVEILLANCE': '15,000 ft',
            'SAR': '1,000 ft',
            'INTERDICTION': '20,000 ft',
            'ASW': '500 ft',
            'PATROL': '10,000 ft',
            'RECON': '25,000 ft'
        };
        
        return altitudes[operationType] || '10,000 ft';
    }

    // Generate mission objectives
    generateObjectives(operationType, threatLevel, patrolArea) {
        const objectiveTemplates = {
            'SURVEILLANCE': [
                'Conduct maritime surveillance of designated patrol area',
                'Monitor and report all surface contacts',
                'Identify and classify unknown vessels',
                'Provide real-time intelligence updates'
            ],
            'SAR': [
                'Search designated area for missing personnel/vessel',
                'Coordinate with rescue assets',
                'Provide on-scene command if required',
                'Maintain communication with rescue coordination center'
            ],
            'INTERDICTION': [
                'Intercept and identify target vessel',
                'Enforce maritime boundaries and regulations',
                'Coordinate with surface assets as required',
                'Document all activities for intelligence purposes'
            ],
            'ASW': [
                'Conduct anti-submarine warfare patrol',
                'Deploy sonobuoys in designated pattern',
                'Investigate all subsurface contacts',
                'Coordinate with allied ASW assets'
            ],
            'PATROL': [
                'Conduct routine maritime patrol',
                'Monitor shipping lanes for irregularities',
                'Respond to distress calls as able',
                'Maintain situational awareness'
            ],
            'RECON': [
                'Conduct reconnaissance of specified targets',
                'Gather intelligence on maritime activities',
                'Photograph/record significant findings',
                'Report time-sensitive information immediately'
            ]
        };
        
        const baseObjectives = objectiveTemplates[operationType] || objectiveTemplates['PATROL'];
        
        // Add threat-specific objectives
        if (threatLevel === 'HIGH' || threatLevel === 'CRITICAL') {
            baseObjectives.push('Maintain heightened defensive posture');
            baseObjectives.push('Report any hostile activities immediately');
        }
        
        return baseObjectives;
    }

    // Generate communication frequencies and codes
    generateCommunications() {
        return {
            primary: `${(118 + Math.random() * 18).toFixed(3)} MHz`,
            secondary: `${(136 + Math.random() * 18).toFixed(3)} MHz`,
            guard: '121.500 MHz',
            satcom: `SATCOM-${Math.floor(Math.random() * 9) + 1}`,
            authentication: this.generateAuthCode(),
            iff: {
                mode1: Math.floor(Math.random() * 100).toString().padStart(2, '0'),
                mode3: Math.floor(Math.random() * 10000).toString().padStart(4, '0')
            }
        };
    }

    // Generate authentication code
    generateAuthCode() {
        const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
        return Array.from({length: 6}, () => chars[Math.floor(Math.random() * chars.length)]).join('');
    }

    // Generate weather conditions
    generateWeather() {
        const conditions = ['CLEAR', 'PARTLY CLOUDY', 'OVERCAST', 'LIGHT RAIN', 'MODERATE RAIN'];
        const winds = [`${Math.floor(Math.random() * 360).toString().padStart(3, '0')}°/${Math.floor(Math.random() * 25 + 5)}KT`];
        const visibility = [`${Math.floor(Math.random() * 8 + 3)}SM`];
        const ceiling = [`${Math.floor(Math.random() * 20000 + 5000)}FT`];
        
        return {
            condition: conditions[Math.floor(Math.random() * conditions.length)],
            wind: winds[0],
            visibility: visibility[0],
            ceiling: ceiling[0],
            temperature: `${Math.floor(Math.random() * 30 + 5)}°C`,
            pressure: `${(29.50 + Math.random() * 1.0).toFixed(2)} inHg`
        };
    }

    // Generate mission timing
    generateTiming() {
        const now = new Date();
        const missionStart = new Date(now.getTime() + (30 * 60 * 1000)); // 30 minutes from now
        
        const durations = {
            'SHORT': 3,
            'STANDARD': 5,
            'EXTENDED': 7,
            'LONG_RANGE': 10
        };
        
        const durationHours = durations[this.currentMission?.duration] || 5;
        const missionEnd = new Date(missionStart.getTime() + (durationHours * 60 * 60 * 1000));
        
        return {
            start: missionStart.toISOString(),
            end: missionEnd.toISOString(),
            duration: `${durationHours}:00`,
            timezone: 'UTC'
        };
    }

    // Generate formatted mission briefing
    generateBriefing() {
        if (!this.currentMission) return null;
        
        const mission = this.currentMission;
        const startTime = new Date(mission.timing.start);
        
        return `
═══════════════════════════════════════════════════════════════
                    OPERATION ${mission.id}
                   MISSION BRIEFING
═══════════════════════════════════════════════════════════════

CLASSIFICATION: OFFICIAL USE ONLY

TIME OF BRIEFING: ${new Date().toISOString().substr(0, 19)}Z
MISSION START: ${startTime.toISOString().substr(0, 19)}Z
ESTIMATED DURATION: ${mission.timing.duration}

═══════════════════════════════════════════════════════════════
                        SITUATION
═══════════════════════════════════════════════════════════════

MISSION TYPE: ${mission.operationType}
THREAT LEVEL: ${mission.threatLevel}
OPERATING AREA: ${mission.patrolArea.name}
WEATHER: ${mission.weather.condition}, Winds ${mission.weather.wind}
VISIBILITY: ${mission.weather.visibility}, Ceiling ${mission.weather.ceiling}

═══════════════════════════════════════════════════════════════
                        MISSION
═══════════════════════════════════════════════════════════════

CALLSIGN: ${mission.callsign}
AIRCRAFT: ${mission.aircraft.type} (${mission.aircraft.tail})
CREW: ${mission.aircraft.crew} Personnel
FUEL LOAD: ${mission.aircraft.fuel.toLocaleString()} lbs

DEPARTURE BASE: ${mission.base.name} (${mission.base.icao})
PATROL AREA: ${mission.patrolArea.name}
COORDINATES: ${mission.patrolArea.coordinates[0].toFixed(4)}°N ${Math.abs(mission.patrolArea.coordinates[1]).toFixed(4)}°${mission.patrolArea.coordinates[1] >= 0 ? 'E' : 'W'}
PATROL RADIUS: ${mission.patrolArea.radius} nm
PATROL ALTITUDE: ${mission.patrolArea.altitude}

═══════════════════════════════════════════════════════════════
                     OBJECTIVES
═══════════════════════════════════════════════════════════════

${mission.objectives.map((obj, i) => `${i + 1}. ${obj}`).join('\n')}

═══════════════════════════════════════════════════════════════
                   COMMUNICATIONS
═══════════════════════════════════════════════════════════════

PRIMARY FREQ: ${mission.communications.primary}
SECONDARY FREQ: ${mission.communications.secondary}
GUARD FREQ: ${mission.communications.guard}
SATCOM: ${mission.communications.satcom}

IFF CODES:
MODE 1: ${mission.communications.iff.mode1}
MODE 3: ${mission.communications.iff.mode3}

AUTHENTICATION: ${mission.communications.authentication}

═══════════════════════════════════════════════════════════════
                      WEAPONS
═══════════════════════════════════════════════════════════════

${mission.aircraft.weapons.map(weapon => `• ${weapon}`).join('\n')}

═══════════════════════════════════════════════════════════════
                   SPECIAL INSTRUCTIONS
═══════════════════════════════════════════════════════════════

• Monitor GUARD frequency at all times
• Report all surface contacts via tactical data link
• Maintain positive aircraft control in controlled airspace
• Execute EMCON procedures as directed
• ROE: Weapons tight unless directed otherwise

FLIR CAMERA CONTROLS:
• F9: Activate FLIR Camera System
• Mouse: Pan/Tilt Camera Controls
• SPACEBAR: Target Lock/Designation
• T: Toggle Thermal/Visual Modes

═══════════════════════════════════════════════════════════════
                      SIGNATURES
═══════════════════════════════════════════════════════════════

MISSION COMMANDER: _________________________

PILOT IN COMMAND: __________________________

BRIEFED BY: MPOC Operations Center

═══════════════════════════════════════════════════════════════

END OF BRIEFING
        `.trim();
    }

    // Get current mission summary for display
    getMissionSummary() {
        if (!this.currentMission) return null;
        
        const mission = this.currentMission;
        return {
            id: mission.id,
            callsign: mission.callsign,
            type: mission.operationType,
            aircraft: mission.aircraft.type,
            area: mission.patrolArea.name,
            status: 'BRIEFED',
            startTime: new Date(mission.timing.start).toLocaleTimeString(),
            duration: mission.timing.duration
        };
    }
}