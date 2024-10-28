echo "Cleaning folders"

rm -rf ./.build/*
rm -rf ./.cache/*
rm *.log

if [[ $1 == "" ]]; then 
    echo "Inserire cartella del progetto da compilare"
fi

echo "Compile"
arduino-cli compile --build-path ./.build/. --build-cache-path ./.cache/. --fqbn esp32:esp32:esp32 ./$1/ > ./$1_compile_attempt.log

if [[ 0 -ne ${PIPESTATUS[0]} ]]; then 
    echo "Ci sono stati errori durante la compilazione del programma"
    exit 1
fi

echo "Upload"
sudo arduino-cli upload --input-dir ./.build/. -p /dev/ttyUSB0  --fqbn esp32:esp32:esp32 ./test/ >> ./$1_compile_attempt.log

if [[ 0 -ne ${PIPESTATUS[0]} ]]; then 
    echo "Ci sono stati errori durante l'upload del programma"
    exit 1
fi

sudo arduino-cli monitor -p /dev/ttyUSB0

exit 0

