// Main Application Controller
class MissionPlannerApp {
    constructor() {
        this.missionGenerator = new MissionGenerator();
        this.mapController = null;
        this.currentMission = null;
        
        this.initializeApp();
    }

    initializeApp() {
        // Wait for DOM to be ready
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', () => this.setup());
        } else {
            this.setup();
        }
    }

    setup() {
        // Initialize map controller
        this.mapController = new MapController('worldMap');
        
        // Make map controller globally accessible for popup callbacks
        window.mapController = this.mapController;
        
        // Populate airbase dropdown
        this.populateAirbaseDropdown();
        
        // Setup event listeners
        this.setupEventListeners();
        
        // Initialize UI state
        this.updatePatrolAreaOptions();
        this.updateAircraftOptions();
        
        console.log('Maritime Patrol Mission Generator initialized');
    }

    populateAirbaseDropdown() {
        const select = document.getElementById('airbaseSelect');
        if (!select) return;

        // Clear existing options except the first one
        select.innerHTML = '<option value="">Select Airbase...</option>';

        // Group airbases by country
        const groupedAirbases = {};
        Object.entries(MILITARY_AIRBASES).forEach(([icao, airbase]) => {
            if (!groupedAirbases[airbase.country]) {
                groupedAirbases[airbase.country] = [];
            }
            groupedAirbases[airbase.country].push({ icao, ...airbase });
        });

        // Sort countries and add options
        Object.keys(groupedAirbases).sort().forEach(country => {
            const optgroup = document.createElement('optgroup');
            optgroup.label = country;

            groupedAirbases[country]
                .sort((a, b) => a.name.localeCompare(b.name))
                .forEach(airbase => {
                    const option = document.createElement('option');
                    option.value = airbase.icao;
                    option.textContent = `${airbase.icao} - ${airbase.name}`;
                    optgroup.appendChild(option);
                });

            select.appendChild(optgroup);
        });
    }

    setupEventListeners() {
        // Airbase selection
        const airbaseSelect = document.getElementById('airbaseSelect');
        if (airbaseSelect) {
            airbaseSelect.addEventListener('change', (e) => {
                this.onAirbaseSelected(e.target.value);
            });
        }

        // Mission type selection
        const missionTypeSelect = document.getElementById('missionType');
        if (missionTypeSelect) {
            missionTypeSelect.addEventListener('change', () => {
                this.updateAircraftOptions();
            });
        }

        // Patrol area selection
        const patrolAreaSelect = document.getElementById('patrolArea');
        if (patrolAreaSelect) {
            patrolAreaSelect.addEventListener('change', (e) => {
                this.onPatrolAreaSelected(e.target.value);
            });
        }

        // Generate mission button
        const generateBtn = document.getElementById('generateMission');
        if (generateBtn) {
            generateBtn.addEventListener('click', () => {
                this.generateMission();
            });
        }

        // Export briefing button
        const exportBtn = document.getElementById('exportBriefing');
        if (exportBtn) {
            exportBtn.addEventListener('click', () => {
                this.exportBriefing();
            });
        }
    }

    onAirbaseSelected(icao) {
        if (!icao) {
            this.clearAirbaseInfo();
            this.updatePatrolAreaOptions();
            this.updateAircraftOptions();
            return;
        }

        const airbase = MILITARY_AIRBASES[icao];
        if (!airbase) return;

        // Update airbase info display
        this.displayAirbaseInfo(icao, airbase);
        
        // Update patrol area options based on airbase
        this.updatePatrolAreaOptions(icao);
        
        // Update aircraft options based on airbase
        this.updateAircraftOptions();
        
        // Highlight on map
        this.mapController.highlightSelectedAirbase(icao);
        this.mapController.fitToAirbase(icao);
    }

    displayAirbaseInfo(icao, airbase) {
        const infoDiv = document.getElementById('airbaseInfo');
        if (!infoDiv) return;

        infoDiv.innerHTML = `
            <div class="info-item">
                <span class="info-label">Country:</span> ${airbase.country}
            </div>
            <div class="info-item">
                <span class="info-label">Region:</span> ${airbase.region.replace('_', ' ')}
            </div>
            <div class="info-item">
                <span class="info-label">Coordinates:</span> ${airbase.coords[0].toFixed(3)}°N, ${Math.abs(airbase.coords[1]).toFixed(3)}°${airbase.coords[1] < 0 ? 'W' : 'E'}
            </div>
            <div class="info-item">
                <span class="info-label">Aircraft:</span> ${airbase.aircraft.slice(0, 2).join(', ')}${airbase.aircraft.length > 2 ? '...' : ''}
            </div>
            <div class="info-item" style="margin-top: 8px; font-size: 10px; color: var(--text-secondary);">
                ${airbase.description}
            </div>
        `;
        
        infoDiv.classList.add('show');
    }

    clearAirbaseInfo() {
        const infoDiv = document.getElementById('airbaseInfo');
        if (infoDiv) {
            infoDiv.classList.remove('show');
        }
    }

    updatePatrolAreaOptions(selectedAirbaseIcao = null) {
        const select = document.getElementById('patrolArea');
        if (!select) return;

        // Clear existing options
        select.innerHTML = '<option value="">Auto-select based on airbase</option>';

        if (!selectedAirbaseIcao) return;

        const airbase = MILITARY_AIRBASES[selectedAirbaseIcao];
        if (!airbase) return;

        // Get appropriate patrol areas
        const { primary, secondary } = getPatrolAreasForAirbase(airbase);
        
        // Add primary areas
        if (primary.length > 0) {
            const primaryGroup = document.createElement('optgroup');
            primaryGroup.label = 'Primary Areas';
            
            primary.forEach(areaKey => {
                const area = PATROL_AREAS[areaKey];
                if (area) {
                    const option = document.createElement('option');
                    option.value = areaKey;
                    option.textContent = area.name;
                    primaryGroup.appendChild(option);
                }
            });
            
            select.appendChild(primaryGroup);
        }

        // Add secondary areas
        if (secondary.length > 0) {
            const secondaryGroup = document.createElement('optgroup');
            secondaryGroup.label = 'Secondary Areas';
            
            secondary.forEach(areaKey => {
                const area = PATROL_AREAS[areaKey];
                if (area) {
                    const option = document.createElement('option');
                    option.value = areaKey;
                    option.textContent = area.name;
                    secondaryGroup.appendChild(option);
                }
            });
            
            select.appendChild(secondaryGroup);
        }

        // Auto-select the first primary area
        if (primary.length > 0) {
            select.value = primary[0];
            this.onPatrolAreaSelected(primary[0]);
        }
    }

    updateAircraftOptions() {
        const airbaseSelect = document.getElementById('airbaseSelect');
        const aircraftSelect = document.getElementById('aircraftType');
        const missionTypeSelect = document.getElementById('missionType');
        
        if (!aircraftSelect || !airbaseSelect) return;

        aircraftSelect.innerHTML = '<option value="">Auto-select based on airbase</option>';

        const selectedAirbase = airbaseSelect.value;
        if (!selectedAirbase) return;

        const airbase = MILITARY_AIRBASES[selectedAirbase];
        if (!airbase || !airbase.aircraft) return;

        // Add aircraft options
        airbase.aircraft.forEach(aircraft => {
            const option = document.createElement('option');
            option.value = aircraft;
            option.textContent = aircraft;
            aircraftSelect.appendChild(option);
        });

        // Auto-select best aircraft for mission type
        const missionType = missionTypeSelect.value;
        if (missionType) {
            const bestAircraft = this.missionGenerator.selectAircraft(airbase.aircraft, missionType);
            aircraftSelect.value = bestAircraft;
        }
    }

    onPatrolAreaSelected(areaKey) {
        if (areaKey) {
            this.mapController.showPatrolArea(areaKey);
        } else {
            this.mapController.clearPatrolAreas();
        }
    }

    generateMission() {
        const airbaseIcao = document.getElementById('airbaseSelect').value;
        const missionType = document.getElementById('missionType').value;
        const patrolArea = document.getElementById('patrolArea').value;

        if (!airbaseIcao) {
            alert('Please select an airbase first.');
            return;
        }

        // Auto-select patrol area if not specified
        let selectedPatrolArea = patrolArea;
        if (!selectedPatrolArea) {
            const airbase = MILITARY_AIRBASES[airbaseIcao];
            const areas = getPatrolAreasForAirbase(airbase);
            selectedPatrolArea = areas.primary[0];
            
            // Update UI
            document.getElementById('patrolArea').value = selectedPatrolArea;
            this.onPatrolAreaSelected(selectedPatrolArea);
        }

        try {
            // Generate the mission
            this.currentMission = this.missionGenerator.generateMission(
                airbaseIcao, 
                missionType, 
                selectedPatrolArea
            );

            // Display the briefing
            this.displayBriefing(this.currentMission);
            
            // Show export button
            document.getElementById('exportBriefing').style.display = 'block';

        } catch (error) {
            console.error('Mission generation failed:', error);
            alert('Failed to generate mission. Please check your selections.');
        }
    }

    displayBriefing(mission) {
        const briefingDiv = document.getElementById('missionBriefing');
        if (!briefingDiv) return;

        const briefingText = this.missionGenerator.generateBriefing(mission);
        
        briefingDiv.innerHTML = `
            <div class="mission-briefing">
                <pre>${briefingText}</pre>
            </div>
        `;
    }

    exportBriefing() {
        if (!this.currentMission) {
            alert('No mission generated to export.');
            return;
        }

        const briefingText = this.missionGenerator.generateBriefing(this.currentMission);
        const filename = `Mission_${this.currentMission.id}_${this.currentMission.codename.replace(' ', '_')}.txt`;
        
        // Create download link
        const blob = new Blob([briefingText], { type: 'text/plain' });
        const url = window.URL.createObjectURL(blob);
        const link = document.createElement('a');
        link.href = url;
        link.download = filename;
        
        // Trigger download
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
        window.URL.revokeObjectURL(url);
    }
}

// Initialize application when page loads
let app;
document.addEventListener('DOMContentLoaded', () => {
    app = new MissionPlannerApp();
});