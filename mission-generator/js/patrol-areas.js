// Patrol Areas Database with intelligent region matching
const PATROL_AREAS = {
    'North_Sea': {
        name: 'North Sea Patrol Zone',
        bounds: { 
            north: 62.0, 
            south: 51.0, 
            east: 8.0, 
            west: -4.0 
        },
        center: [56.5, 2.0],
        description: 'High-traffic shipping lane surveillance and fishery protection',
        threats: ['Heavy commercial traffic', 'Weather deterioration', 'Fishing vessel interference'],
        keyAreas: [
            'Dogger Bank fishing grounds',
            'Norwegian shipping lanes',
            'UK-Netherlands ferry routes',
            'Oil platform approaches'
        ]
    },
    
    'English_Channel': {
        name: 'English Channel Transit Zone',
        bounds: { 
            north: 51.5, 
            south: 49.0, 
            east: 2.5, 
            west: -5.0 
        },
        center: [50.25, -1.25],
        description: 'Critical maritime chokepoint monitoring and cross-channel surveillance',
        threats: ['Dense commercial traffic', 'Multiple national jurisdictions', 'Restricted military areas'],
        keyAreas: [
            'Dover Strait separation scheme',
            'Portsmouth-Le Havre ferry routes',
            'Channel Islands approaches',
            'Cherbourg naval approaches'
        ]
    },
    
    'Atlantic': {
        name: 'North Atlantic Patrol Area',
        bounds: { 
            north: 65.0, 
            south: 35.0, 
            east: -5.0, 
            west: -70.0 
        },
        center: [50.0, -37.5],
        description: 'Long-range maritime patrol and search and rescue operations',
        threats: ['Extreme weather conditions', 'Long transit times', 'Limited diversion airfields'],
        keyAreas: [
            'GIUK Gap surveillance',
            'Trans-Atlantic shipping lanes',
            'Grand Banks fishing areas',
            'Mid-Atlantic Ridge approaches'
        ]
    },
    
    'Mediterranean': {
        name: 'Mediterranean Security Zone',
        bounds: { 
            north: 45.0, 
            south: 30.0, 
            east: 36.0, 
            west: -6.0 
        },
        center: [37.5, 15.0],
        description: 'Central and Eastern Mediterranean patrol operations',
        threats: ['International tensions', 'Migration routes', 'Multiple sovereign waters'],
        keyAreas: [
            'Strait of Gibraltar approaches',
            'Central Mediterranean corridor',
            'Aegean Sea patrol zones',
            'Libyan coast surveillance'
        ]
    },
    
    'Gulf_of_Mexico': {
        name: 'Gulf of Mexico Operations Area',
        bounds: { 
            north: 31.0, 
            south: 18.0, 
            east: -80.0, 
            west: -98.0 
        },
        center: [24.5, -89.0],
        description: 'Gulf coast security and hurricane response operations',
        threats: ['Hurricane season', 'Oil platform density', 'Drug interdiction operations'],
        keyAreas: [
            'Mississippi Delta approaches',
            'Texas offshore platforms',
            'Florida Keys corridor',
            'Yucatan Channel surveillance'
        ]
    },
    
    'Pacific': {
        name: 'Pacific Operations Area',
        bounds: { 
            north: 50.0, 
            south: 20.0, 
            east: -120.0, 
            west: 120.0 
        },
        center: [35.0, -180.0],
        description: 'Wide-area Pacific maritime surveillance and patrol',
        threats: ['Vast distances', 'Weather systems', 'International waters complexity'],
        keyAreas: [
            'Hawaiian approaches',
            'Alaska fishing grounds',
            'West Coast shipping lanes',
            'Trans-Pacific routes'
        ]
    },
    
    'Arctic': {
        name: 'Arctic Maritime Zone',
        bounds: { 
            north: 85.0, 
            south: 66.5, 
            east: 180.0, 
            west: -180.0 
        },
        center: [75.0, 0.0],
        description: 'Arctic Ocean surveillance and search and rescue',
        threats: ['Extreme cold conditions', 'Limited infrastructure', 'Ice navigation hazards'],
        keyAreas: [
            'Northwest Passage routes',
            'Barents Sea approaches',
            'Greenland Sea patrol zones',
            'Svalbard approaches'
        ]
    },
    
    'Indian_Ocean': {
        name: 'Indian Ocean Operations Area',
        bounds: { 
            north: 30.0, 
            south: -50.0, 
            east: 120.0, 
            west: 30.0 
        },
        center: [-10.0, 75.0],
        description: 'Indian Ocean maritime security and anti-piracy operations',
        threats: ['Piracy activity', 'Monsoon weather patterns', 'Long transit distances'],
        keyAreas: [
            'Arabian Sea patrol zones',
            'Bay of Bengal surveillance',
            'Strait of Hormuz approaches',
            'Mozambique Channel security',
            'Malacca Strait monitoring',
            'Somali Basin anti-piracy'
        ]
    }
};

// Function to determine appropriate patrol areas based on airbase location
function getPatrolAreasForAirbase(airbaseData) {
    const { region, coords } = airbaseData;
    const [lat, lon] = coords;
    
    // Primary region assignment
    const primaryAreas = [region];
    
    // Add secondary areas based on geographic proximity
    const secondaryAreas = [];
    
    // Logic for secondary area assignment
    switch (region) {
        case 'North_Sea':
            if (lat > 55) secondaryAreas.push('Arctic');
            if (lon < 0) secondaryAreas.push('Atlantic');
            secondaryAreas.push('English_Channel');
            break;
            
        case 'English_Channel':
            secondaryAreas.push('North_Sea', 'Atlantic');
            if (lat < 50) secondaryAreas.push('Mediterranean');
            break;
            
        case 'Atlantic':
            if (lat > 55) secondaryAreas.push('Arctic');
            if (lat < 45) secondaryAreas.push('Mediterranean');
            if (lon > -20) secondaryAreas.push('North_Sea');
            if (lon < -60) secondaryAreas.push('Gulf_of_Mexico');
            break;
            
        case 'Mediterranean':
            secondaryAreas.push('Atlantic');
            if (lat > 42) secondaryAreas.push('English_Channel');
            break;
            
        case 'Pacific':
            if (lat > 45) secondaryAreas.push('Arctic');
            if (lon > -130 && lat < 35) secondaryAreas.push('Gulf_of_Mexico');
            break;
            
        case 'Gulf_of_Mexico':
            secondaryAreas.push('Atlantic');
            if (lon < -90) secondaryAreas.push('Pacific');
            break;
            
        case 'Arctic':
            if (lon > -30 && lon < 30) secondaryAreas.push('North_Sea');
            secondaryAreas.push('Atlantic', 'Pacific');
            break;
            
        case 'Indian_Ocean':
            if (lat > 20) secondaryAreas.push('Mediterranean');
            if (lon > 100) secondaryAreas.push('Pacific');
            if (lon < 50) secondaryAreas.push('Atlantic');
            break;
    }
    
    return {
        primary: primaryAreas,
        secondary: secondaryAreas.filter(area => !primaryAreas.includes(area))
    };
}

// Function to calculate distance between airbase and patrol area center
function calculateDistanceToPatrolArea(airbaseCoords, patrolAreaKey) {
    const area = PATROL_AREAS[patrolAreaKey];
    if (!area) return Infinity;
    
    const [lat1, lon1] = airbaseCoords;
    const [lat2, lon2] = area.center;
    
    // Haversine formula for great circle distance
    const R = 6371; // Earth's radius in kilometers
    const dLat = (lat2 - lat1) * Math.PI / 180;
    const dLon = (lon2 - lon1) * Math.PI / 180;
    
    const a = Math.sin(dLat/2) * Math.sin(dLat/2) +
              Math.cos(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) *
              Math.sin(dLon/2) * Math.sin(dLon/2);
    
    const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
    const distance = R * c;
    
    return Math.round(distance);
}

// Function to generate patrol waypoints within an area
function generatePatrolWaypoints(patrolAreaKey, numWaypoints = 4) {
    const area = PATROL_AREAS[patrolAreaKey];
    if (!area) return [];
    
    const waypoints = [];
    const { bounds } = area;
    
    // Generate waypoints in a rough patrol pattern
    for (let i = 0; i < numWaypoints; i++) {
        const latRange = bounds.north - bounds.south;
        const lonRange = bounds.east - bounds.west;
        
        // Create a roughly rectangular patrol pattern
        let lat, lon;
        
        if (i === 0) {
            // Northwest corner
            lat = bounds.south + latRange * 0.8;
            lon = bounds.west + lonRange * 0.2;
        } else if (i === 1) {
            // Northeast corner
            lat = bounds.south + latRange * 0.8;
            lon = bounds.west + lonRange * 0.8;
        } else if (i === 2) {
            // Southeast corner
            lat = bounds.south + latRange * 0.2;
            lon = bounds.west + lonRange * 0.8;
        } else {
            // Southwest corner
            lat = bounds.south + latRange * 0.2;
            lon = bounds.west + lonRange * 0.2;
        }
        
        waypoints.push({
            name: `PATROL_${i + 1}`,
            coords: [parseFloat(lat.toFixed(3)), parseFloat(lon.toFixed(3))],
            description: `Patrol waypoint ${i + 1}`
        });
    }
    
    return waypoints;
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { 
        PATROL_AREAS, 
        getPatrolAreasForAirbase, 
        calculateDistanceToPatrolArea,
        generatePatrolWaypoints 
    };
}