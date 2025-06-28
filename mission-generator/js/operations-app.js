// Main Operations Application
// Coordinates all tactical operations center functionality

class OperationsApp {
    constructor() {
        this.missionController = new MissionController();
        this.tacticalMap = null;
        this.currentTime = new Date();
        this.selectedBaseId = null;
        
        this.initialize();
    }

    // Initialize the operations application
    initialize() {
        this.initializeMap();
        this.populateBaseSelector();
        this.setupEventListeners();
        this.startTimeUpdate();
        this.updateWeather();
        this.setupMapControls();
        
        console.log('MPOC Operations Center Initialized');
    }

    // Initialize the tactical map
    initializeMap() {
        this.tacticalMap = new TacticalMap('tacticalMap');
        
        // Override base selection handler
        this.tacticalMap.onBaseSelected = (id, base) => {
            this.handleBaseSelection(id, base);
        };
    }

    // Populate the base selection dropdown
    populateBaseSelector() {
        const baseSelect = document.getElementById('baseSelect');
        
        // Group bases by type
        const groupedBases = this.groupBasesByType();
        
        Object.entries(groupedBases).forEach(([type, bases]) => {
            const optgroup = document.createElement('optgroup');
            optgroup.label = type.replace('_', ' ') + ' BASES';
            
            bases.forEach(([id, base]) => {
                const option = document.createElement('option');
                option.value = id;
                option.textContent = `${base.name} (${base.icao || 'N/A'}) - ${base.country}`;
                optgroup.appendChild(option);
            });
            
            baseSelect.appendChild(optgroup);
        });
    }

    // Group bases by type for organized display
    groupBasesByType() {
        const grouped = {};
        
        Object.entries(TACTICAL_BASES).forEach(([id, base]) => {
            const type = base.type;
            if (!grouped[type]) {
                grouped[type] = [];
            }
            grouped[type].push([id, base]);
        });
        
        // Sort each group by name
        Object.keys(grouped).forEach(type => {
            grouped[type].sort((a, b) => a[1].name.localeCompare(b[1].name));
        });
        
        return grouped;
    }

    // Setup event listeners
    setupEventListeners() {
        // Base selection
        document.getElementById('baseSelect').addEventListener('change', (e) => {
            if (e.target.value) {
                const base = TACTICAL_BASES[e.target.value];
                this.handleBaseSelection(e.target.value, base);
                this.tacticalMap.selectBase(e.target.value, base);
            } else {
                this.clearBaseSelection();
            }
        });

        // Mission generation
        document.getElementById('generateOperation').addEventListener('click', () => {
            this.generateOperation();
        });

        // Briefing modal
        document.getElementById('briefingMode').addEventListener('click', () => {
            this.showBriefingModal();
        });

        document.getElementById('closeBriefing').addEventListener('click', () => {
            this.closeBriefingModal();
        });

        document.getElementById('acknowledgeBriefing').addEventListener('click', () => {
            this.acknowledgeBriefing();
        });

        document.getElementById('printBriefing').addEventListener('click', () => {
            this.printBriefing();
        });
    }

    // Setup map control buttons
    setupMapControls() {
        document.getElementById('showBases').addEventListener('click', (e) => {
            e.target.classList.toggle('active');
            this.tacticalMap.toggleBases(e.target.classList.contains('active'));
        });

        document.getElementById('showRoutes').addEventListener('click', (e) => {
            e.target.classList.toggle('active');
            this.tacticalMap.toggleRoutes(e.target.classList.contains('active'));
        });

        document.getElementById('showThreats').addEventListener('click', (e) => {
            e.target.classList.toggle('active');
            this.tacticalMap.toggleThreats(e.target.classList.contains('active'));
        });

        // Initialize with bases shown
        document.getElementById('showBases').classList.add('active');
    }

    // Handle base selection
    handleBaseSelection(id, base) {
        this.selectedBaseId = id;
        this.updateBaseDetails(base);
        this.updateAircraftAssignment(base);
        this.enableMissionGeneration();
    }

    // Clear base selection
    clearBaseSelection() {
        this.selectedBaseId = null;
        this.tacticalMap.clearSelection();
        document.getElementById('baseDetails').style.display = 'none';
        document.getElementById('aircraftAssignment').innerHTML = 
            '<div class="aircraft-placeholder">Select base to view available aircraft</div>';
        this.disableMissionGeneration();
    }

    // Update base details display
    updateBaseDetails(base) {
        const detailsDiv = document.getElementById('baseDetails');
        
        detailsDiv.innerHTML = `
            <div class="base-detail-item">
                <span class="detail-label">TYPE:</span>
                <span class="detail-value">${base.type.replace('_', ' ')}</span>
            </div>
            <div class="base-detail-item">
                <span class="detail-label">ICAO:</span>
                <span class="detail-value">${base.icao || 'N/A'}</span>
            </div>
            <div class="base-detail-item">
                <span class="detail-label">ELEVATION:</span>
                <span class="detail-value">${base.elevation || 'N/A'}</span>
            </div>
            <div class="base-detail-item">
                <span class="detail-label">RUNWAY:</span>
                <span class="detail-value">${base.runway || 'N/A'}</span>
            </div>
            <div class="base-detail-item">
                <span class="detail-label">FREQUENCY:</span>
                <span class="detail-value">${base.frequency || 'N/A'}</span>
            </div>
        `;
        
        detailsDiv.classList.add('show');
    }

    // Update aircraft assignment display
    updateAircraftAssignment(base) {
        const assignmentDiv = document.getElementById('aircraftAssignment');
        const aircraftTypes = BASE_AIRCRAFT[base.type] || BASE_AIRCRAFT['AIR_FORCE'];
        
        const primaryAircraft = aircraftTypes.PRIMARY[0];
        const tailNumber = this.generateTailNumber();
        
        assignmentDiv.innerHTML = `
            <div class="aircraft-info">
                <div class="aircraft-type">${primaryAircraft}</div>
                <div style="color: #9aa0a6; font-size: 9px; margin-top: 3px;">
                    Tail: ${tailNumber} | Status: READY | Crew: ASSIGNED
                </div>
            </div>
        `;
    }

    // Generate aircraft tail number
    generateTailNumber() {
        const letters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
        const numbers = '0123456789';
        
        return Array.from({length: 2}, () => letters[Math.floor(Math.random() * letters.length)]).join('') +
               '-' +
               Array.from({length: 3}, () => numbers[Math.floor(Math.random() * numbers.length)]).join('');
    }

    // Enable mission generation
    enableMissionGeneration() {
        document.getElementById('generateOperation').disabled = false;
    }

    // Disable mission generation
    disableMissionGeneration() {
        document.getElementById('generateOperation').disabled = true;
        this.clearMissionDisplay();
    }

    // Generate new operation
    generateOperation() {
        if (!this.selectedBaseId) {
            alert('Please select a base first');
            return;
        }

        const operationType = document.getElementById('operationType').value;
        const threatLevel = document.getElementById('threatLevel').value;
        const duration = document.getElementById('patrolDuration').value;

        // Generate mission
        const mission = this.missionController.generateMission(
            this.selectedBaseId,
            operationType,
            threatLevel,
            duration
        );

        if (mission) {
            this.updateMissionDisplay(mission);
            this.tacticalMap.addMissionOverlay(mission);
            this.updateMissionStatus('ACTIVE');
            this.showBriefingButton();
        }
    }

    // Update mission display
    updateMissionDisplay(mission) {
        const missionDiv = document.getElementById('currentMission');
        
        // Shorten aircraft type if too long
        const aircraftType = mission.aircraft.type.length > 15 ? 
            mission.aircraft.type.substring(0, 12) + '...' : 
            mission.aircraft.type;
            
        // Shorten patrol area if too long  
        const patrolArea = mission.patrolArea.name.length > 12 ?
            mission.patrolArea.name.substring(0, 9) + '...' :
            mission.patrolArea.name;
            
        // Format start time as HH:MM
        const startTime = new Date(mission.timing.start).toLocaleTimeString('en-US', {
            hour12: false,
            hour: '2-digit',
            minute: '2-digit'
        });
        
        missionDiv.innerHTML = `
            <div style="color: #00ff7f; font-weight: bold; margin-bottom: 10px; font-size: 11px;">
                OPERATION ${mission.id}
            </div>
            <div class="intel-item">
                <span class="intel-label">CALLSIGN:</span>
                <span class="intel-value">${mission.callsign}</span>
            </div>
            <div class="intel-item">
                <span class="intel-label">TYPE:</span>
                <span class="intel-value">${mission.operationType}</span>
            </div>
            <div class="intel-item">
                <span class="intel-label">AIRCRAFT:</span>
                <span class="intel-value" title="${mission.aircraft.type}">${aircraftType}</span>
            </div>
            <div class="intel-item">
                <span class="intel-label">AREA:</span>
                <span class="intel-value" title="${mission.patrolArea.name}">${patrolArea}</span>
            </div>
            <div class="intel-item">
                <span class="intel-label">START:</span>
                <span class="intel-value">${startTime}Z</span>
            </div>
            <div class="intel-item">
                <span class="intel-label">DURATION:</span>
                <span class="intel-value">${mission.timing.duration}</span>
            </div>
        `;
    }

    // Clear mission display
    clearMissionDisplay() {
        const missionDiv = document.getElementById('currentMission');
        missionDiv.innerHTML = `
            <div class="mission-placeholder">
                <div class="placeholder-icon">🎯</div>
                <p>No active mission</p>
                <p>Select base and generate operation to begin</p>
            </div>
        `;
        this.hideBriefingButton();
        this.updateMissionStatus('STANDBY');
    }

    // Update mission status
    updateMissionStatus(status) {
        const statusDiv = document.getElementById('missionStatus');
        statusDiv.textContent = status;
        
        // Update status color
        statusDiv.className = 'mission-status';
        if (status === 'ACTIVE') {
            statusDiv.style.color = '#00ff7f';
            statusDiv.style.borderColor = '#00ff7f';
        } else {
            statusDiv.style.color = '#ff8c00';
            statusDiv.style.borderColor = '#ff8c00';
        }
    }

    // Show briefing button
    showBriefingButton() {
        document.getElementById('briefingMode').style.display = 'block';
    }

    // Hide briefing button
    hideBriefingButton() {
        document.getElementById('briefingMode').style.display = 'none';
    }

    // Show briefing modal
    showBriefingModal() {
        const briefing = this.missionController.generateBriefing();
        if (briefing) {
            document.getElementById('briefingContent').innerHTML = `<pre>${briefing}</pre>`;
            document.getElementById('briefingModal').style.display = 'flex';
        }
    }

    // Close briefing modal
    closeBriefingModal() {
        document.getElementById('briefingModal').style.display = 'none';
    }

    // Acknowledge briefing
    acknowledgeBriefing() {
        this.updateMissionStatus('ACKNOWLEDGED');
        this.closeBriefingModal();
        
        // Could trigger mission start procedures here
        console.log('Mission briefing acknowledged');
    }

    // Print briefing
    printBriefing() {
        const briefingContent = document.getElementById('briefingContent').innerText;
        const printWindow = window.open('', '_blank');
        
        printWindow.document.write(`
            <html>
                <head>
                    <title>Mission Briefing</title>
                    <style>
                        body { font-family: 'Courier New', monospace; font-size: 12px; margin: 20px; }
                        pre { white-space: pre-wrap; }
                    </style>
                </head>
                <body>
                    <pre>${briefingContent}</pre>
                </body>
            </html>
        `);
        
        printWindow.document.close();
        printWindow.print();
    }

    // Start time update interval
    startTimeUpdate() {
        setInterval(() => {
            this.updateCurrentTime();
        }, 1000);
        
        this.updateCurrentTime(); // Initial update
    }

    // Update current time display
    updateCurrentTime() {
        this.currentTime = new Date();
        const timeElement = document.getElementById('currentTime');
        if (timeElement) {
            timeElement.textContent = this.currentTime.toISOString().substr(11, 8) + 'Z';
        }
    }

    // Update weather display (simulated)
    updateWeather() {
        const conditions = ['CLEAR', 'PARTLY CLOUDY', 'OVERCAST', 'SCATTERED'];
        const condition = conditions[Math.floor(Math.random() * conditions.length)];
        
        document.getElementById('currentWeather').textContent = condition;
        document.getElementById('weatherStatus').textContent = condition;
        
        // Update other intel
        document.getElementById('seaState').textContent = ['CALM', 'SLIGHT', 'MODERATE'][Math.floor(Math.random() * 3)];
        document.getElementById('visibility').textContent = ['EXCELLENT', 'GOOD', 'FAIR'][Math.floor(Math.random() * 3)];
        
        // Update periodically
        setTimeout(() => this.updateWeather(), 300000); // 5 minutes
    }
}

// Initialize application when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    window.operationsApp = new OperationsApp();
});