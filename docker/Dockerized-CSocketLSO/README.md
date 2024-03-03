Per costruire questa immagine docker apri il cmd dentro questa cartella contente il dockerfile e scrivi il comando:
    docker build -t csocketlso .

Per runnare il container di questa immagine scrivere:
    docker run --network=docker_default csocketlso
