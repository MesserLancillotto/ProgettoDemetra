package main

import (
    "bufio"
    "fmt"
    "log"
    "net"
	"time"
)

func main() {
    listener, err := net.Listen("tcp", ":4020")
    if err != nil {
        log.Fatalf("Error starting TCP server: %v", err)
    }
    defer listener.Close()

    fmt.Println("Ade is listening on port 4020")

    for {
        conn, err := listener.Accept()
        if err != nil {
            log.Println("Error accepting connection:", err)
            continue
        }
        fmt.Println("Demetra connected")
        go handleConnection(conn)
    }
}


func handleConnection(conn net.Conn) {
    defer conn.Close()
    reader := bufio.NewReader(conn)
    for {
        message, err := reader.ReadString('\n')
        if err != nil {
            log.Println("Demetra disconnected or error reading:", err)
            break
        }
		// -- time -- //
		currentTime := time.Now()
		gmtPlus1 := time.FixedZone("GMT+1", 1*60*60)
		currentTimeInGMTPlus1 := currentTime.In(gmtPlus1)
		formattedTime := currentTimeInGMTPlus1.Format("2006-01-02 15:04:05")
        // Print the received message
        fmt.Printf("%s: %s", formattedTime, message)
    }
}
