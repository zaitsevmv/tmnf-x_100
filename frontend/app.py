from flask import Flask, render_template, jsonify, request
import datetime
import os

app = Flask(__name__)

data_store = {
    "original": {
        "tracks_without_replays": {"count": 10},
        "replays_since_last_update": {"count": 5, "last_update": ""},
        "leaderboards": {},
        "all_category": {}
    },
    "difficulties": {
        "leaderboards": {}
    }
}

def parse_leaderboards_from_lines(lines):
    """Parse leaderboard lines and return (categories_dict, all_category_dict)"""
    leaderboards = {}
    all_category = {}
    current_category = None
    
    for line in lines:
        line = line.strip()
        if not line or line == "=":
            current_category = None
            continue
        
        if current_category is None:
            current_category = line
            if current_category == "All":
                all_category = {}
            else:
                leaderboards[current_category] = {}
        else:
            parts = line.rsplit(maxsplit=1)
            if len(parts) != 2:
                continue
            player, score = parts
            try:
                score = int(score)
                if current_category == "All":
                    all_category[player] = score
                else:
                    leaderboards[current_category][player] = score
            except ValueError:
                continue

    for category in leaderboards:
        leaderboards[category] = dict(
            sorted(leaderboards[category].items(), key=lambda x: x[1], reverse=True)
        )
        print(leaderboards[category])
    if all_category:
        all_category = dict(sorted(all_category.items(), key=lambda x: x[1], reverse=True))
    
    return leaderboards, all_category

def get_original_data():
    filepath = "../data/data_to_frontend_tag.txt"
    with open(filepath, "r") as file:
        lines = file.readlines()
    
    # Parse first 3 lines for stats
    if len(lines) >= 3:
        last_update = lines[0].strip()
        try:
            last_update_dt = datetime.datetime.strptime(last_update, "%a %b %d %H:%M:%S %Y")
            time_since = datetime.datetime.now() - last_update_dt
            if time_since.total_seconds() > 3600:
                last_update_str = f"{time_since.total_seconds() / 3600:.0f} hours ago"
            elif time_since.total_seconds() > 60:
                last_update_str = f"{time_since.total_seconds() / 60:.0f} minutes ago"
            else:
                last_update_str = f"{time_since.total_seconds():.0f} seconds ago"
        except:
            last_update_str = last_update
        
        data_store["original"]["replays_since_last_update"]["last_update"] = last_update_str
        data_store["original"]["tracks_without_replays"]["count"] = int(lines[1].strip())
        data_store["original"]["replays_since_last_update"]["count"] = int(lines[2].strip())
    
    # Parse remaining lines for leaderboards
    leaderboard_lines = lines[3:] if len(lines) > 3 else []
    leaderboards, all_cat = parse_leaderboards_from_lines(leaderboard_lines)
    data_store["original"]["leaderboards"] = leaderboards
    data_store["original"]["all_category"] = all_cat

def get_difficulties_data():
    """Fetch and parse difficulties.txt (leaderboards only)"""
    filepath = "../data/data_to_frontend_difficulty.txt"
    if not os.path.exists(filepath):
        return
    
    with open(filepath, "r") as file:
        lines = file.readlines()
    
    leaderboards, _ = parse_leaderboards_from_lines(lines)
    data_store["difficulties"]["leaderboards"] = leaderboards

@app.route('/')
def index():
    get_original_data()
    get_difficulties_data()
    return render_template('index.html')

@app.route('/api/tracks_without_replays', methods=['GET'])
def api_tracks_without_replays():
    get_original_data()
    return jsonify(data_store["original"]["tracks_without_replays"])

@app.route('/api/replays_since_last_update', methods=['GET'])
def api_replays_since_last_update():
    get_original_data()
    return jsonify(data_store["original"]["replays_since_last_update"])

@app.route('/api/leaderboards', methods=['GET'])
def api_leaderboards():
    source = request.args.get('source', 'original')
    
    if source == "difficulties":
        get_difficulties_data()
        return jsonify({
            "leaderboards": data_store["difficulties"]["leaderboards"],
            "all_category": None
        })
    elif source == "all":
        get_original_data()
        return jsonify({
            "leaderboards": {},
            "all_category": data_store["original"]["all_category"]
        })
    else:
        get_original_data()
        return jsonify({
            "leaderboards": data_store["original"]["leaderboards"],
            "all_category": data_store["original"]["all_category"]
        })

if __name__ == '__main__':
    app.run(debug=True)