import uuid
import random
from locust import HttpUser, task, between


def extract_id(resp):
    try:
        data = resp.json()
        if isinstance(data, dict):
            for key in ["id", "teamId", "groupId", "matchId", "tournamentId"]:
                v = data.get(key)
                if isinstance(v, (str, int)):
                    return str(v)
        elif isinstance(data, list) and data:
            first = data[0]
            if isinstance(first, dict):
                for key in ["id", "teamId", "groupId", "matchId", "tournamentId"]:
                    v = first.get(key)
                    if isinstance(v, (str, int)):
                        return str(v)
    except Exception:
        pass
    loc = resp.headers.get("location") or resp.headers.get("Location")
    if loc:
        return loc.rstrip("/").split("/")[-1]
    return None


class TeamsUser(HttpUser):
    wait_time = between(0.1, 0.3)

    def create_team(self):
        name = f"Team {uuid.uuid4()}"
        payload = {"name": name}
        with self.client.post(
                "/teams",
                json=payload,
                catch_response=True,
                name="POST /teams",
        ) as resp:
            if resp.status_code >= 400:
                resp.failure(f"Team creation failed: {resp.status_code}")
                return None
            team_id = extract_id(resp)
            return team_id

    def update_team(self, team_id):
        if not team_id:
            return
        payload = {"name": f"Team updated {uuid.uuid4()}"}
        with self.client.patch(
                f"/teams/{team_id}",
                json=payload,
                catch_response=True,
                name="PATCH /teams/{id}",
        ) as resp:
            if resp.status_code == 405:
                resp.success()
                return
            if resp.status_code >= 400:
                resp.failure(f"Team update failed: {resp.status_code}")

    @task
    def create_and_update_team(self):
        team_id = self.create_team()
        self.update_team(team_id)


class TournamentUser(HttpUser):
    wait_time = between(0.1, 0.3)

    def create_team(self):
        name = f"Team {uuid.uuid4()}"
        payload = {"name": name}
        with self.client.post(
                "/teams",
                json=payload,
                catch_response=True,
                name="POST /teams",
        ) as resp:
            if resp.status_code >= 400:
                resp.failure(f"Team creation failed: {resp.status_code}")
                return None
            team_id = extract_id(resp)
            if team_id:
                return {"id": team_id, "name": name}
            return None

    def create_teams(self, count=8):
        teams = []
        for _ in range(count):
            t = self.create_team()
            if t:
                teams.append(t)
        return teams

    def create_tournament(self):
        payload = {
            "name": f"Tournament {uuid.uuid4()}",
            "groupsCount": 1,
            "maxTeamsPerGroup": 32,
        }
        with self.client.post(
                "/tournaments",
                json=payload,
                catch_response=True,
                name="POST /tournaments",
        ) as resp:
            if resp.status_code >= 400:
                resp.failure(f"Tournament creation failed: {resp.status_code}")
                return None
            tournament_id = extract_id(resp)
            return tournament_id

    def create_group(self, tournament_id):
        if not tournament_id:
            return None
        payload = {"name": f"Group {uuid.uuid4()}"}
        with self.client.post(
                f"/tournaments/{tournament_id}/groups",
                json=payload,
                catch_response=True,
                name="POST /tournaments/{tid}/groups",
        ) as resp:
            if resp.status_code >= 400:
                resp.failure(f"Group creation failed: {resp.status_code}")
                return None
            group_id = extract_id(resp)
            return group_id

    def assign_teams_to_group(self, tournament_id, group_id, teams):
        if not tournament_id or not group_id or not teams:
            return
        payload = [{"id": t["id"], "name": t["name"]} for t in teams]
        with self.client.patch(
                f"/tournaments/{tournament_id}/groups/{group_id}/teams",
                json=payload,
                catch_response=True,
                name="PATCH /tournaments/{tid}/groups/{gid}/teams",
        ) as resp:
            if resp.status_code >= 400:
                resp.failure(f"Update teams in group failed: {resp.status_code}")

    def get_matches(self, tournament_id):
        if not tournament_id:
            return []
        with self.client.get(
                f"/tournaments/{tournament_id}/matches",
                catch_response=True,
                name="GET /tournaments/{tid}/matches",
        ) as resp:
            if resp.status_code == 404:
                resp.success()
                return []
            if resp.status_code >= 400:
                resp.failure(f"Get matches failed: {resp.status_code}")
                return []
            try:
                data = resp.json()
            except Exception:
                resp.failure("Get matches: invalid JSON")
                return []
            match_ids = []
            if isinstance(data, list):
                for item in data:
                    if isinstance(item, dict):
                        mid = item.get("id") or item.get("matchId")
                        if isinstance(mid, (str, int)):
                            match_ids.append(str(mid))
            return match_ids

    def update_match_score(self, tournament_id, match_id):
        if not tournament_id or not match_id:
            return
        payload = {
            "score": {
                "home": random.randint(0, 5),
                "visitor": random.randint(0, 5),
            }
        }
        with self.client.patch(
                f"/tournaments/{tournament_id}/matches/{match_id}",
                json=payload,
                catch_response=True,
                name="PATCH /tournaments/{tid}/matches/{mid}",
        ) as resp:
            if resp.status_code >= 400:
                resp.failure(f"Update match score failed: {resp.status_code}")

    @task
    def full_tournament_lifecycle(self):
        teams = self.create_teams(count=8)
        tournament_id = self.create_tournament()
        group_id = self.create_group(tournament_id)
        self.assign_teams_to_group(tournament_id, group_id, teams)
        match_ids = self.get_matches(tournament_id)
        for mid in match_ids:
            self.update_match_score(tournament_id, mid)
