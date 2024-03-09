# CSoketLSO

## Database
1. Pullare l'immagine "postgres" ufficiale:
    
    `docker pull postgres`

2. Per creare e far partire un container da questo file yaml aprire il cmd dentro questa cartella e scrivere il comando:
    
    `docker-compose -f database.yml up`

- Per far partire il container già creato:
    
    `docker start lsodb`

- Per eseguire comandi SQL dentro questo container:
    
    `docker exec -it <container_id> psql -U <username> -d <database> -c "<SQL command>"`


## Server
1. Per costruire l' immagine docker della socket rimanere in questa cartella contente il dockerfile e scrivere il comando:

    `docker build -t csocketlso .`

2. Per far partire il container del server scrivere:
    
    `docker-compose -f Dockerized-CSocketLSO.yml up`