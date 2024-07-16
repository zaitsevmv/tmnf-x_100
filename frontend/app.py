from flask import Flask, render_template, jsonify
import datetime

app = Flask(__name__)

data_store = {
    "tracks_without_replays": {"count": 10},
    "replays_since_last_update": {"count": 5, "last_update": ""},
    "leaderboards": {}
}

def get_data():
    with open("data.txt", "r") as file:
        last_update = file.readline().strip()
        last_update = datetime.datetime.strptime(last_update, "%a %b %d %H:%M:%S %Y")
        time_since = datetime.datetime.now() - last_update

        if time_since.total_seconds() > 3600:
            last_update_str = f"{time_since.total_seconds() / 3600:.0f} hours ago"
        elif time_since.total_seconds() > 60:
            last_update_str = f"{time_since.total_seconds() / 60:.0f} minutes ago"
        else:
            last_update_str = f"{time_since.total_seconds()} seconds ago"

        data_store["replays_since_last_update"]["last_update"] = last_update_str

        no_replays = file.readline().strip()
        data_store["tracks_without_replays"]["count"] = int(no_replays)

        replays_since = file.readline().strip()
        data_store["replays_since_last_update"]["count"] = int(replays_since)

        data_store["leaderboards"] = {}
        current_category = None

        for line in file:
            line = line.strip()

            if not line:
                continue

            if line == "=":
                current_category = None
                continue

            if current_category is None:
                current_category = line
                data_store["leaderboards"][current_category] = {}
            else:
                player, score = line.rsplit(maxsplit=1)
                data_store["leaderboards"][current_category][player] = int(score)

    for category in data_store["leaderboards"]:
        sorted_leaderboard = dict(sorted(data_store["leaderboards"][category].items(), key=lambda item: item[1], reverse=True))
        data_store["leaderboards"][category] = sorted_leaderboard
    print(data_store)

@app.route('/')
def index():
    get_data()
    return render_template('index.html')

@app.route('/api/tracks_without_replays', methods=['GET'])
def api_tracks_without_replays():
    get_data()
    return jsonify(data_store["tracks_without_replays"])

@app.route('/api/replays_since_last_update', methods=['GET'])
def api_replays_since_last_update():
    get_data()
    return jsonify(data_store["replays_since_last_update"])

@app.route('/api/leaderboards', methods=['GET'])
def api_leaderboards():
    get_data()
    return jsonify(data_store["leaderboards"])

if __name__ == '__main__':
    app.run(debug=True)
