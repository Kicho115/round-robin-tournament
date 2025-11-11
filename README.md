
DB script
````
podman exec -i tournament_db psql -U postgres -d postgres < db_script.sql
````

activemq
````
podman run -d --replace --name artemis --network Dev -p 61616:61616 -p 8161:8161 -p 5672:5672 -m 256m  apache/activemq-classic:6.1.7
````
Load Balancer commands
````
podman build -f tournament_services/Containerfile -t tournament_services

podman run --replace -d --network Dev --name tournament_services_1 -p 8081:8080 -m 256m tournament_services
podman run --replace -d --network Dev --name tournament_services_2 -p 8082:8080 -m 256m tournament_services
podman run --replace -d --network Dev --name tournament_services_3 -p 8083:8080 -m 256m tournament_services

podman run -d --replace --name load_balancer --network Dev -p 8000:8080 -p 8404:8404 -v ./tournament_services/haproxy.cfg:/usr/local/etc/haproxy/haproxy.cfg:Z haproxy
````