# CSoketLSO

## Database
1. Pullare l'immagine "postgres" ufficiale:
    
    `docker pull postgres`

2. Per creare e far partire un container da questo file yaml scrivere nel cmd il comando:
    
    `docker-compose -f database.yml up`

- Per far partire il container già creato:
    
    `docker start lsodb`

- Per eseguire comandi SQL dentro questo container:
    
    `docker exec -it <container_id> psql -U <username> -d <database> -c "<SQL command>"`


## Server
1. creare l'immagine del server CSocketLSO entrando nella cartella "Dockerized-CSocketLSO" e 
   seguendo le istruzioni del readme che sta lì dentro

2. tornare qui e far partire il container del server tramite:
    
    `docker-compose -f Dockerized-CSocketLSO.yml up`