import requests
import json
import os
from duckduckgo_search import DDGS
from geopy.geocoders import Nominatim
import folium
from bs4 import BeautifulSoup
import httpx

# ── Weather & Flood Tools ─────────────────────────────────────────────────────

def get_weather_data(city="Dhaka"):
    """Fetch current weather from Open-Meteo."""
    try:
        geolocator = Nominatim(user_agent="marin_bayazid_assistant")
        location = geolocator.geocode(city)
        if not location:
            return {"error": f"Could not find coordinates for {city}"}
        
        lat, lon = location.latitude, location.longitude
        url = f"https://api.open-meteo.com/v1/forecast?latitude={lat}&longitude={lon}&current_weather=true&hourly=relative_humidity_2m"
        r = requests.get(url, timeout=10)
        r.raise_for_status()
        data = r.json()
        
        current = data.get("current_weather", {})
        # Extract humidity if available (hourly data usually)
        humidity = data.get("hourly", {}).get("relative_humidity_2m", [None])[0]
        
        return {
            "city": city,
            "latitude": lat,
            "longitude": lon,
            "temperature": current.get("temperature"),
            "windspeed": current.get("windspeed"),
            "weathercode": current.get("weathercode"),
            "humidity": humidity,
            "time": current.get("time")
        }
    except Exception as e:
        return {"error": str(e)}

def get_flood_data():
    """Fetch recent flood events from NASA EONET."""
    try:
        # EONET Category 15 is 'Floods'
        url = "https://eonet.gsfc.nasa.gov/api/v3/events?category=floods&status=open"
        r = requests.get(url, timeout=10)
        r.raise_for_status()
        events = r.json().get("events", [])
        
        results = []
        for e in events:
            results.append({
                "title": e.get("title"),
                "date": e.get("geometry", [{}])[0].get("date"),
                "coordinates": e.get("geometry", [{}])[0].get("coordinates")
            })
        return results
    except Exception as e:
        return {"error": str(e)}

def get_route_data(start_city, end_city):
    """Fetch routing information between two cities."""
    try:
        geolocator = Nominatim(user_agent="marin_bayazid_assistant")
        start_loc = geolocator.geocode(start_city)
        end_loc = geolocator.geocode(end_city)
        
        if not start_loc or not end_loc:
            return {"error": "Could not find coordinates for one or both cities"}
        
        # Using OSRM public API for routing (OpenStreetMap based)
        url = f"http://router.project-osrm.org/route/v1/driving/{start_loc.longitude},{start_loc.latitude};{end_loc.longitude},{end_loc.latitude}?overview=full&geometries=geojson"
        r = requests.get(url, timeout=10)
        r.raise_for_status()
        data = r.json()
        
        if data.get("code") != "Ok":
            return {"error": "Routing failed"}
        
        route = data["routes"][0]
        return {
            "distance_km": round(route["distance"] / 1000, 2),
            "duration_mins": round(route["duration"] / 60, 2),
            "geometry": route["geometry"],
            "start_coords": [start_loc.latitude, start_loc.longitude],
            "end_coords": [end_loc.latitude, end_loc.longitude]
        }
    except Exception as e:
        return {"error": str(e)}

def create_integrated_hub_map(city="Dhaka", destination=None):
    """Generate a map with weather, floods, and optional route."""
    weather = get_weather_data(city)
    floods = get_flood_data()
    
    if "error" in weather:
        return f"Error creating map: {weather['error']}"

    m = folium.Map(location=[weather["latitude"], weather["longitude"]], zoom_start=8)
    
    # Weather Marker
    weather_html = f"""
    <div style="font-family: Arial; width: 200px;">
        <h4>Weather in {city}</h4>
        <b>Temp:</b> {weather['temperature']}°C<br>
        <b>Humidity:</b> {weather['humidity']}%<br>
        <b>Wind:</b> {weather['windspeed']} km/h<br>
    </div>
    """
    folium.Marker(
        [weather["latitude"], weather["longitude"]],
        popup=folium.Popup(weather_html, max_width=300),
        tooltip=f"Current Weather: {city}",
        icon=folium.Icon(color='blue', icon='cloud')
    ).add_to(m)
    
    # Route
    route_info = None
    if destination:
        route_data = get_route_data(city, destination)
        if "error" not in route_data:
            route_info = route_data
            # Draw Route
            folium.GeoJson(
                route_data["geometry"],
                name="Route",
                style_function=lambda x: {"color": "blue", "weight": 5}
            ).add_to(m)
            # Destination Marker
            folium.Marker(
                route_data["end_coords"],
                popup=f"Destination: {destination}",
                icon=folium.Icon(color='green', icon='flag')
            ).add_to(m)
    
    # Flood Markers
    for f in floods:
        if f.get("coordinates"):
            lon, lat = f["coordinates"]
            folium.Marker(
                [lat, lon],
                popup=f"Flood Event: {f['title']}",
                tooltip="NASA EONET Flood Data",
                icon=folium.Icon(color='red', icon='info-sign')
            ).add_to(m)
    
    map_path = os.path.join("static", "generated", "knowledge_hub_map.html")
    os.makedirs(os.path.dirname(map_path), exist_ok=True)
    m.save(map_path)
    return {
        "map_url": f"/static/generated/knowledge_hub_map.html",
        "weather": weather,
        "floods": floods,
        "route": route_info
    }

# ── Web Search & Scrape Tools ────────────────────────────────────────────────

def search_web(query, max_results=5):
    """Search the web using DuckDuckGo."""
    try:
        with DDGS() as ddgs:
            results = [r for r in ddgs.text(query, max_results=max_results)]
            return results
    except Exception as e:
        return {"error": str(e)}

def search_pdfs(topic):
    """Specialized search for PDFs/Books."""
    query = f"{topic} filetype:pdf"
    return search_web(query, max_results=10)

async def scrape_content(url):
    """Scrape web content using Jina Reader (dynamic) or BeautifulSoup (static)."""
    try:
        # Try Jina Reader first for clean markdown output
        jina_url = f"https://r.jina.ai/{url}"
        async with httpx.AsyncClient(follow_redirects=True) as client:
            r = await client.get(jina_url, timeout=20)
            if r.status_code == 200:
                return r.text
            
            # Fallback to BeautifulSoup
            r = await client.get(url, timeout=15)
            r.raise_for_status()
            soup = BeautifulSoup(r.text, 'html.parser')
            # Remove script and style elements
            for s in soup(["script", "style"]):
                s.decompose()
            return soup.get_text(separator="\n", strip=True)[:5000] # Limit output
    except Exception as e:
        return f"Scraping failed: {e}"

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Knowledge Hub CLI")
    parser.add_argument("--city", type=str, default="Dhaka", help="City name")
    parser.add_argument("--destination", type=str, help="Destination city for routing")
    parser.add_argument("--search", type=str, help="Web search query")
    parser.add_argument("--pdf", type=str, help="PDF search topic")

    args = parser.parse_args()

    if args.search:
        print(json.dumps(search_web(args.search), indent=2))
    elif args.pdf:
        print(json.dumps(search_pdfs(args.pdf), indent=2))
    elif args.destination:
        print(json.dumps(create_integrated_hub_map(args.city, args.destination), indent=2))
    else:
        print(json.dumps(get_weather_data(args.city), indent=2))

