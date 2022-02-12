package main
import (
    "log"
    "github.com/tarm/serial"
    "github.com/google/uuid"
    "github.com/syndtr/goleveldb/leveldb"
    "time"
    "os"
    "bufio"
    "strings"
)
func readFromConsole() chan string {
    ch := make(chan string)
    go func(ch chan string) {
        reader:= bufio.NewReader(os.Stdin)
        for {
            text, err:= reader.ReadString('\n')
            if err != nil {
                close(ch)
                return
            }
            text = strings.Replace(text, "\n", "", -1)
            ch <- text
        } 
    }(ch)
    return ch
}

type  Sensors struct{
    humidity float32
    tempC float32
    waterTempC float32
    tds float32
    ph float32
    lectureCompleted bool
    buffer []byte 
}
func (sensors *Sensors) SetHumidity(h float32) {
    sensors.humidity = h
}
func (sensors *Sensors) SetTempC(t float32) {
    sensors.tempC = t
}
func (sensors *Sensors) SetWaterTempC(t float32) {
    sensors.waterTempC = t
}
func (sensors *Sensors) SetTDS(tds float32) {
    sensors.tds = tds
}
func (sensors *Sensors) SetPH(ph float32) {
    sensors.ph = ph
}
func (sensors *Sensors) SaveToBuffer(reading []byte) {
    copy(sensors.buffer, reading)
}
func OpenLevelDB () *leveldb.DB {
    db, err := leveldb.OpenFile("./sensor_data", nil)
   Check(err) 
    return db
}
func Check(err error) {
    if err !=nil {
        log.Fatal(err)
    }
}
func main () {
    db := OpenLevelDB()
    connection := &serial.Config{Name: "/dev/cu.usbmodem1433401", Baud:115200, ReadTimeout: time.Second *2 }
    stream, err := serial.OpenPort(connection)
    Check(err)
    buf := make([]byte, 256)
    reading :=make([]byte, 256) 
    ch := readFromConsole()
    bytesInReading := 0
    stream.Flush()
    stdinloop:
        for {
            select {
                case stdin, ok := <-ch:
                    if !ok {
                        break stdinloop
                    } else {
                        if strings.Compare("y", stdin) == 0 {
                            break stdinloop
                        }
                    }
                case <- time.After(2*time.Second):
                    bytesRead, err := stream.Read(buf)
                    Check(err)
                    if strings.Compare("\n", string(buf[bytesRead-1])) == 0 {
                        copy(reading[bytesInReading:], buf[:bytesRead-1])
                        totalBytes := bytesInReading+bytesRead
                        if totalBytes == 48 {
                            id := uuid.New().String()
                            errUpdate := db.Put([]byte(id),reading[:bytesInReading+bytesRead], nil)
                            Check(errUpdate) 
                            data, errRead := db.Get([]byte(id), nil)
                            Check(errRead) 
                            log.Printf("%v",string(data))
                        }
                        reading = make([]byte, 256)
                        bytesInReading = 0
                    }else{
                        copy(reading[bytesInReading:], buf[:bytesRead])
                        bytesInReading +=bytesRead
                    }
            }
        }
    log.Fatal(stream.Close())
}
