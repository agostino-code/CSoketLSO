# CSoketLSO

1. Pullare l'immagine "postgres" ufficiale:
    docker pull postgres

2. Per creare e far partire un container da questo file yaml scrivere nel cmd il comando:
    docker-compose -f dietidealsDB.yaml up

3. Rinominare il database:
    docker rename docker-db-1 dietidealsdb
   O in generale:
    docker rename <old_container_name> <new_container_name>

Per far partire il container già creato:
    docker start dietidealsdb

Per eseguire comandi SQL dentro questo container:
    docker exec -it <container_id> psql -U <username> -d <database> -c "<SQL command>"
Esempio:
    docker exec -it dietidealsdb psql -U postgres -d dietidealsdb -c "SELECT* FROM Users;"