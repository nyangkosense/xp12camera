// Map Controller for Leaflet Integration
class MapController {
    constructor(mapElementId) {
        this.mapElementId = mapElementId;
        this.map = null;
        this.airbaseMarkers = [];
        this.patrolAreaLayers = [];
        this.selectedAirbase = null;
        this.selectedPatrolArea = null;
        
        this.initializeMap();
        this.loadAirbases();
    }

    initializeMap() {
        // Initialize map centered on Europe/North Atlantic
        this.map = L.map(this.mapElementId, {
            center: [55.0, -5.0],
            zoom: 4,
            minZoom: 2,
            maxZoom: 10,
            worldCopyJump: true
        });

        // Dark theme tile layer
        L.tileLayer('https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png', {
            attribution: '&copy; <a href="https://carto.com/attributions">CARTO</a>',
            subdomains: 'abcd',
            maxZoom: 19
        }).addTo(this.map);

        // Add custom CSS for dark theme
        this.addMapStyles();
    }

    addMapStyles() {
        const style = document.createElement('style');
        style.textContent = `
            .leaflet-popup-content-wrapper {
                background: var(--secondary-bg);
                color: var(--text-primary);
                border: 1px solid var(--accent-green);
                border-radius: 3px;
            }
            
            .leaflet-popup-tip {
                background: var(--secondary-bg);
                border: 1px solid var(--accent-green);
            }
            
            .airbase-popup {
                font-family: 'Courier New', monospace;
                font-size: 11px;
                line-height: 1.4;
                min-width: 200px;
            }
            
            .airbase-popup .popup-header {
                color: var(--accent-green);
                font-weight: bold;
                margin-bottom: 8px;
                text-align: center;
                border-bottom: 1px solid var(--border-color);
                padding-bottom: 5px;
            }
            
            .airbase-popup .popup-info {
                margin-bottom: 5px;
            }
            
            .airbase-popup .popup-label {
                color: var(--text-secondary);
                display: inline-block;
                width: 60px;
            }
            
            .airbase-popup .popup-aircraft {
                color: var(--warning-amber);
                font-size: 10px;
                margin-top: 5px;
                padding: 3px;
                background: var(--tertiary-bg);
                border-radius: 2px;
            }
            
            .select-airbase-btn {
                background: var(--military-blue);
                color: var(--text-primary);
                border: 1px solid var(--accent-green);
                padding: 5px 10px;
                font-family: inherit;
                font-size: 10px;
                cursor: pointer;
                width: 100%;
                margin-top: 8px;
                border-radius: 2px;
                transition: all 0.3s ease;
            }
            
            .select-airbase-btn:hover {
                background: var(--accent-green);
                color: var(--primary-bg);
            }
        `;
        document.head.appendChild(style);
    }

    loadAirbases() {
        // Clear existing markers
        this.airbaseMarkers.forEach(marker => this.map.removeLayer(marker));
        this.airbaseMarkers = [];

        // Create markers for each airbase
        Object.entries(MILITARY_AIRBASES).forEach(([icao, airbase]) => {
            const marker = this.createAirbaseMarker(icao, airbase);
            this.airbaseMarkers.push(marker);
            marker.addTo(this.map);
        });
    }

    createAirbaseMarker(icao, airbase) {
        const [lat, lon] = airbase.coords;
        
        // Custom airbase icon
        const airbaseIcon = L.divIcon({
            className: 'airbase-marker',
            html: `<div style="
                width: 8px; 
                height: 8px; 
                background: var(--accent-green); 
                border: 2px solid var(--primary-bg);
                border-radius: 50%; 
                box-shadow: 0 0 8px rgba(0, 255, 65, 0.6);
            "></div>`,
            iconSize: [12, 12],
            iconAnchor: [6, 6]
        });

        const marker = L.marker([lat, lon], { icon: airbaseIcon });

        // Create popup content
        const popupContent = this.createAirbasePopup(icao, airbase);
        marker.bindPopup(popupContent, {
            maxWidth: 250,
            className: 'airbase-popup-container'
        });

        // Add click handler
        marker.on('click', () => {
            this.selectAirbase(icao);
        });

        return marker;
    }

    createAirbasePopup(icao, airbase) {
        const aircraftList = Array.isArray(airbase.aircraft) ? 
            airbase.aircraft.slice(0, 3).join(', ') + 
            (airbase.aircraft.length > 3 ? '...' : '') : 
            'Various aircraft';

        return `
            <div class="airbase-popup">
                <div class="popup-header">${airbase.name}</div>
                <div class="popup-info">
                    <span class="popup-label">ICAO:</span> ${icao}
                </div>
                <div class="popup-info">
                    <span class="popup-label">Country:</span> ${airbase.country}
                </div>
                <div class="popup-info">
                    <span class="popup-label">Region:</span> ${airbase.region.replace('_', ' ')}
                </div>
                <div class="popup-info">
                    <span class="popup-label">Coords:</span> ${airbase.coords[0].toFixed(3)}°, ${airbase.coords[1].toFixed(3)}°
                </div>
                <div class="popup-aircraft">
                    Aircraft: ${aircraftList}
                </div>
                <button class="select-airbase-btn" onclick="window.mapController.selectAirbase('${icao}')">
                    SELECT AIRBASE
                </button>
            </div>
        `;
    }

    selectAirbase(icao) {
        this.selectedAirbase = icao;
        const airbase = MILITARY_AIRBASES[icao];
        
        // Update airbase selection in UI
        const airbaseSelect = document.getElementById('airbaseSelect');
        if (airbaseSelect) {
            airbaseSelect.value = icao;
            airbaseSelect.dispatchEvent(new Event('change'));
        }

        // Highlight selected airbase
        this.highlightSelectedAirbase(icao);
        
        // Pan to airbase
        const [lat, lon] = airbase.coords;
        this.map.setView([lat, lon], 6);
        
        // Close popup
        this.map.closePopup();
    }

    highlightSelectedAirbase(icao) {
        // Reset all markers to normal
        this.airbaseMarkers.forEach(marker => {
            const element = marker.getElement();
            if (element) {
                const icon = element.querySelector('div');
                if (icon) {
                    icon.style.background = 'var(--accent-green)';
                    icon.style.boxShadow = '0 0 8px rgba(0, 255, 65, 0.6)';
                }
            }
        });

        // Highlight selected marker
        const selectedMarker = this.airbaseMarkers.find(marker => {
            const airbase = Object.entries(MILITARY_AIRBASES).find(([code, data]) => 
                data.coords[0] === marker.getLatLng().lat && 
                data.coords[1] === marker.getLatLng().lng
            );
            return airbase && airbase[0] === icao;
        });

        if (selectedMarker) {
            const element = selectedMarker.getElement();
            if (element) {
                const icon = element.querySelector('div');
                if (icon) {
                    icon.style.background = 'var(--warning-amber)';
                    icon.style.boxShadow = '0 0 12px rgba(255, 176, 0, 0.8)';
                }
            }
        }
    }

    showPatrolArea(patrolAreaKey) {
        // Clear existing patrol area layers
        this.clearPatrolAreas();
        
        const area = PATROL_AREAS[patrolAreaKey];
        if (!area) return;

        const { bounds } = area;
        
        // Create rectangle for patrol area
        const rectangle = L.rectangle([
            [bounds.south, bounds.west],
            [bounds.north, bounds.east]
        ], {
            color: '#1e3a5f',
            fillColor: '#1e3a5f',
            fillOpacity: 0.1,
            weight: 2,
            opacity: 0.6,
            dashArray: '5, 5'
        });

        rectangle.addTo(this.map);
        this.patrolAreaLayers.push(rectangle);

        // Add label
        const center = area.center;
        const label = L.marker(center, {
            icon: L.divIcon({
                className: 'patrol-area-label',
                html: `<div style="
                    color: var(--military-blue);
                    font-family: 'Courier New', monospace;
                    font-size: 11px;
                    font-weight: bold;
                    text-shadow: 1px 1px 2px rgba(0,0,0,0.8);
                    white-space: nowrap;
                ">${area.name}</div>`,
                iconSize: [120, 20],
                iconAnchor: [60, 10]
            })
        });

        label.addTo(this.map);
        this.patrolAreaLayers.push(label);

        // Fit bounds to show both airbase and patrol area
        if (this.selectedAirbase) {
            const airbase = MILITARY_AIRBASES[this.selectedAirbase];
            const group = new L.featureGroup([rectangle, L.marker(airbase.coords)]);
            this.map.fitBounds(group.getBounds(), { padding: [20, 20] });
        }
    }

    clearPatrolAreas() {
        this.patrolAreaLayers.forEach(layer => this.map.removeLayer(layer));
        this.patrolAreaLayers = [];
    }

    fitToAirbase(icao) {
        const airbase = MILITARY_AIRBASES[icao];
        if (airbase) {
            const [lat, lon] = airbase.coords;
            this.map.setView([lat, lon], 6);
        }
    }

    getMap() {
        return this.map;
    }
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { MapController };
}