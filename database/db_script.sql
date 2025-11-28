-- podman run -d --replace --name=tournament_db --network development -e POSTGRES_PASSWORD=password -p 5432:5432 postgres:17.6-alpine3.22
-- podman exec -i tournament_db psql -U postgres -d postgres < db_script.sql

CREATE USER tournament_svc WITH PASSWORD 'password';
CREATE USER tournament_admin WITH PASSWORD 'password';

CREATE DATABASE tournament_db;

\connect tournament_db

grant all privileges on database tournament_db to tournament_admin;
grant all privileges on database tournament_db to tournament_svc;
grant usage on schema public to tournament_admin;
grant usage on schema public to tournament_svc;

GRANT SELECT ON ALL TABLES IN SCHEMA public TO tournament_admin;
GRANT DELETE ON ALL TABLES IN SCHEMA public TO tournament_admin;
GRANT UPDATE ON ALL TABLES IN SCHEMA public TO tournament_admin;
GRANT INSERT ON ALL TABLES IN SCHEMA public TO tournament_admin;
GRANT CREATE ON SCHEMA public TO tournament_admin;

\connect tournament_db tournament_admin

CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- Tabla de equipos
CREATE TABLE teams (
    id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    document JSONB NOT NULL,
    last_update_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE UNIQUE INDEX team_unique_name_idx ON teams ((document->>'name'));

-- Tabla de torneos
CREATE TABLE tournaments (
    id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    document JSONB NOT NULL,
    last_update_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE UNIQUE INDEX tournament_unique_name_idx ON tournaments ((document->>'name'));

-- Tabla de grupos (con foreign key correcta a tournaments)
CREATE TABLE groups (
    id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    name TEXT,
    tournament_id UUID NOT NULL REFERENCES tournaments(id) ON DELETE CASCADE,
    document JSONB NOT NULL,
    last_update_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE UNIQUE INDEX tournament_group_unique_name_idx ON groups (tournament_id, (document->>'name'));

-- Tabla de relación grupo-equipo
CREATE TABLE group_teams (
    group_id UUID NOT NULL REFERENCES groups(id) ON DELETE CASCADE,
    team_id UUID NOT NULL,
    team_name TEXT NOT NULL,
    PRIMARY KEY (group_id, team_id)
);

-- Tabla de partidos
CREATE TABLE matches (
    id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    document JSONB NOT NULL,
    last_update_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Permisos para tournament_svc (deben estar AL FINAL después de crear todas las tablas)
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO tournament_svc;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO tournament_svc;

-- Asegurar permisos específicos en tablas críticas
GRANT SELECT, INSERT, UPDATE, DELETE ON teams, tournaments, groups, group_teams, matches TO tournament_svc;
