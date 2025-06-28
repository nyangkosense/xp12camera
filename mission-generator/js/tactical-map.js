// Tactical Map Controller
// Handles Leaflet map with military base display and mission overlays

class TacticalMap {
    constructor(containerId) {
        this.containerId = containerId;
        this.map = null;
        this.baseMarkers = new L.LayerGroup();
        this.patrolAreas = new L.LayerGroup();
        this.threatZones = new L.LayerGroup();
        this.flightRoutes = new L.LayerGroup();
        this.selectedBase = null;
        this.currentMission = null;
        
        this.initializeMap();
        this.loadBases();
    }

    // Initialize the Leaflet map
    initializeMap() {
        this.map = L.map(this.containerId, {
            center: [40.0, 0.0],
            zoom: 3,
            zoomControl: true,
            attributionControl: false,
            preferCanvas: true
        });

        // Dark tactical tile layer
        L.tileLayer('https://cartodb-basemaps-{s}.global.ssl.fastly.net/dark_all/{z}/{x}/{y}.png', {
            attribution: '',
            subdomains: 'abcd',
            maxZoom: 18
        }).addTo(this.map);

        // Add layer groups to map
        this.baseMarkers.addTo(this.map);
        this.patrolAreas.addTo(this.map);
        this.threatZones.addTo(this.map);
        this.flightRoutes.addTo(this.map);

        // Custom map controls styling
        this.styleMapControls();
    }

    // Style map controls to match tactical theme
    styleMapControls() {
        const style = document.createElement('style');
        style.textContent = `
            .leaflet-control-zoom {
                border: 1px solid #34495e !important;
                background: rgba(21, 26, 33, 0.9) !important;
                border-radius: 3px !important;
            }
            .leaflet-control-zoom a {
                background: rgba(21, 26, 33, 0.9) !important;
                color: #00ff7f !important;
                border: none !important;
                font-weight: bold !important;
            }
            .leaflet-control-zoom a:hover {
                background: rgba(0, 255, 127, 0.2) !important;
                color: #ffffff !important;
            }
            .leaflet-popup-content-wrapper {
                background: #151a21 !important;
                color: #f1f3f4 !important;
                border: 1px solid #00ff7f !important;
                border-radius: 3px !important;
            }
            .leaflet-popup-tip {
                background: #151a21 !important;
                border: 1px solid #00ff7f !important;
            }
        `;
        document.head.appendChild(style);
    }

    // Load military bases onto the map
    loadBases() {
        Object.entries(TACTICAL_BASES).forEach(([id, base]) => {
            this.addBaseMarker(id, base);
        });
    }

    // Add a military base marker
    addBaseMarker(id, base) {
        const icon = this.createBaseIcon(base.type);
        
        const marker = L.marker(base.coordinates, { icon: icon })
            .bindPopup(this.createBasePopup(id, base))
            .on('click', () => this.selectBase(id, base));
        
        marker.baseId = id;
        marker.baseData = base;
        this.baseMarkers.addLayer(marker);
    }

    // Create custom icons for different base types
    createBaseIcon(baseType) {
        const iconConfigs = {
            'AIR_FORCE': {
                html: '✈',
                className: 'base-icon air-force',
                color: '#00ff7f'
            },
            'NAVAL_AIR': {
                html: '⚓',
                className: 'base-icon naval-air',
                color: '#3742fa'
            },
            'MARINE_AIR': {
                html: '⚡',
                className: 'base-icon marine-air',
                color: '#ff8c00'
            },
            'NAVAL_BASE': {
                html: '⚓',
                className: 'base-icon naval-base',
                color: '#3742fa'
            }
        };

        const config = iconConfigs[baseType] || iconConfigs['AIR_FORCE'];
        
        return L.divIcon({
            html: `<div style="color: ${config.color}; font-size: 16px; text-align: center; line-height: 20px; text-shadow: 0 0 3px ${config.color};">${config.html}</div>`,
            className: config.className,
            iconSize: [20, 20],
            iconAnchor: [10, 10]
        });
    }

    // Create popup content for base
    createBasePopup(id, base) {
        return `
            <div style="font-family: 'Consolas', monospace; font-size: 11px;">
                <div style="color: #00ff7f; font-weight: bold; margin-bottom: 5px;">
                    ${base.name}
                </div>
                <div style="color: #9aa0a6; margin-bottom: 3px;">
                    <strong>ICAO:</strong> ${base.icao || 'N/A'}
                </div>
                <div style="color: #9aa0a6; margin-bottom: 3px;">
                    <strong>Type:</strong> ${base.type.replace('_', ' ')}
                </div>
                <div style="color: #9aa0a6; margin-bottom: 3px;">
                    <strong>Country:</strong> ${base.country}
                </div>
                <div style="color: #9aa0a6; margin-bottom: 5px;">
                    <strong>Elevation:</strong> ${base.elevation || 'N/A'}
                </div>
                <div style="color: #f1f3f4; font-size: 9px;">
                    ${base.capabilities?.join(', ') || 'Standard Operations'}
                </div>
            </div>
        `;
    }

    // Select a base and highlight it
    selectBase(id, base) {
        // Remove previous selection highlight
        this.clearSelection();
        
        // Store selected base
        this.selectedBase = { id, data: base };
        
        // Highlight selected base
        this.highlightBase(id);
        
        // Center map on base
        this.map.setView(base.coordinates, 8);
        
        // Trigger base selection event
        this.onBaseSelected(id, base);
    }

    // Highlight the selected base
    highlightBase(baseId) {
        this.baseMarkers.eachLayer(marker => {
            if (marker.baseId === baseId) {
                // Create highlight circle
                const highlight = L.circle(marker.getLatLng(), {
                    radius: 100000, // 100km radius
                    fillColor: '#00ff7f',
                    color: '#00ff7f',
                    weight: 2,
                    opacity: 0.8,
                    fillOpacity: 0.1
                });
                
                highlight.addTo(this.map);
                marker.highlightCircle = highlight;
            }
        });
    }

    // Clear base selection
    clearSelection() {
        this.baseMarkers.eachLayer(marker => {
            if (marker.highlightCircle) {
                this.map.removeLayer(marker.highlightCircle);
                delete marker.highlightCircle;
            }
        });
        this.selectedBase = null;
    }

    // Add mission patrol area to map
    addMissionOverlay(mission) {
        this.clearMissionOverlay();
        this.currentMission = mission;
        
        // Add patrol area circle
        const patrolCircle = L.circle(mission.patrolArea.coordinates, {
            radius: mission.patrolArea.radius * 1852, // Convert nm to meters
            fillColor: '#ff8c00',
            color: '#ff8c00',
            weight: 2,
            opacity: 0.8,
            fillOpacity: 0.2
        }).bindPopup(this.createPatrolPopup(mission));
        
        this.patrolAreas.addLayer(patrolCircle);
        
        // Add flight route from base to patrol area
        if (this.selectedBase) {
            const route = L.polyline([
                this.selectedBase.data.coordinates,
                mission.patrolArea.coordinates
            ], {
                color: '#00ff7f',
                weight: 3,
                opacity: 0.7,
                dashArray: '10, 10'
            }).bindPopup(`Flight Route: ${mission.callsign}`);
            
            this.flightRoutes.addLayer(route);
        }
        
        // Zoom to show both base and patrol area
        if (this.selectedBase) {
            const group = new L.featureGroup([
                L.marker(this.selectedBase.data.coordinates),
                L.marker(mission.patrolArea.coordinates)
            ]);
            this.map.fitBounds(group.getBounds().pad(0.1));
        }
    }

    // Create patrol area popup
    createPatrolPopup(mission) {
        return `
            <div style="font-family: 'Consolas', monospace; font-size: 11px;">
                <div style="color: #ff8c00; font-weight: bold; margin-bottom: 5px;">
                    PATROL AREA: ${mission.patrolArea.name}
                </div>
                <div style="color: #9aa0a6; margin-bottom: 3px;">
                    <strong>Operation:</strong> ${mission.id}
                </div>
                <div style="color: #9aa0a6; margin-bottom: 3px;">
                    <strong>Callsign:</strong> ${mission.callsign}
                </div>
                <div style="color: #9aa0a6; margin-bottom: 3px;">
                    <strong>Radius:</strong> ${mission.patrolArea.radius} nm
                </div>
                <div style="color: #9aa0a6; margin-bottom: 3px;">
                    <strong>Altitude:</strong> ${mission.patrolArea.altitude}
                </div>
                <div style="color: #9aa0a6;">
                    <strong>Type:</strong> ${mission.operationType}
                </div>
            </div>
        `;
    }

    // Clear mission overlay
    clearMissionOverlay() {
        this.patrolAreas.clearLayers();
        this.flightRoutes.clearLayers();
        this.threatZones.clearLayers();
        this.currentMission = null;
    }

    // Toggle base visibility
    toggleBases(show) {
        if (show) {
            this.map.addLayer(this.baseMarkers);
        } else {
            this.map.removeLayer(this.baseMarkers);
        }
    }

    // Toggle route visibility
    toggleRoutes(show) {
        if (show) {
            this.map.addLayer(this.flightRoutes);
        } else {
            this.map.removeLayer(this.flightRoutes);
        }
    }

    // Toggle threat intel visibility
    toggleThreats(show) {
        if (show) {
            this.map.addLayer(this.threatZones);
            this.loadThreatIntel();
        } else {
            this.map.removeLayer(this.threatZones);
        }
    }

    // Load simulated threat intelligence
    loadThreatIntel() {
        this.threatZones.clearLayers();
        
        // Add some example threat zones
        const threats = [
            {
                name: "Piracy Risk Area",
                coordinates: [12.0, 45.0],
                radius: 50000,
                level: "MEDIUM"
            },
            {
                name: "Restricted Zone",
                coordinates: [35.0, 25.0],
                radius: 30000,
                level: "HIGH"
            }
        ];
        
        threats.forEach(threat => {
            const color = threat.level === 'HIGH' ? '#ff4757' : '#ff8c00';
            
            const threatZone = L.circle(threat.coordinates, {
                radius: threat.radius,
                fillColor: color,
                color: color,
                weight: 2,
                opacity: 0.6,
                fillOpacity: 0.1
            }).bindPopup(`
                <div style="font-family: 'Consolas', monospace; font-size: 11px;">
                    <div style="color: ${color}; font-weight: bold; margin-bottom: 5px;">
                        THREAT: ${threat.name}
                    </div>
                    <div style="color: #9aa0a6;">
                        <strong>Level:</strong> ${threat.level}
                    </div>
                </div>
            `);
            
            this.threatZones.addLayer(threatZone);
        });
    }

    // Event handler for base selection (override in implementation)
    onBaseSelected(id, base) {
        // Override this method to handle base selection
        console.log('Base selected:', id, base);
    }

    // Get currently selected base
    getSelectedBase() {
        return this.selectedBase;
    }

    // Get current mission overlay
    getCurrentMission() {
        return this.currentMission;
    }

    // Zoom to specific coordinates
    zoomTo(coordinates, zoom = 10) {
        this.map.setView(coordinates, zoom);
    }

    // Get map bounds
    getBounds() {
        return this.map.getBounds();
    }
}