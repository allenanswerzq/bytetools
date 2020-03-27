package main

import (
	"fmt"
	"github.com/fatih/color"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"sync"
	"syscall"
	"time"
	"unsafe"
)

func ReadFile(name string) []byte {
	data, err := ioutil.ReadFile(name)
	if err != nil {
		log.Fatal(err)
	}
	return data
}

func WriteFile(name string, data []byte) {
 	f, err := os.OpenFile(name, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
  if err != nil {
    log.Fatal(err)
  }
  if _, err := f.Write(data); err != nil {
    log.Fatal(err)
  }
  if err := f.Close(); err != nil {
    log.Fatal(err)
  }
}


func FileExist(name string) bool {
	_, err := os.Stat(name)
	return !os.IsNotExist(err)
}

func RunCmd(str string, dry_run ...bool) bool {
	str = "gtimeout -s2 5 " + str
	if len(dry_run) == 0 {
		cmd := exec.Command("sh", "-c", str)
		err := cmd.Run()
		if err != nil {
			// TODO(zq7): get the exact exit status of a cmd.
			return false
		}
		cmd.Wait()
	}
	return true
}

type JobState struct {
	stage int
	idx   int
	mutex sync.Mutex
}

var STAGE_GEN int = 0
var STAGE_RUN int = 1
var STAGE_CMP int = 2
var STAGE_DIF int = 3

var job_state JobState
var xy string = "xy"
var xy_cnts int = 4
var xy_done int = 0
var xy_done_mutex sync.Mutex
var xy_channel_send chan bool
var xy_channel_recv chan bool
var xy_kill bool = false
var xy_pass bool = false
var xy_need_compare = false
var xy_kthreds int = 4
var terminal_width int = 0

func DrawSplit(ch string, msg string) {
	tot := terminal_width
	printer := color.New(color.FgRed, color.Bold)
	if len(msg) > 0 {
		msg = " " + msg + " "
		half := (tot - len(msg)) / 2
		printer.Printf(strings.Repeat(ch, half))
		printer.Printf(msg)
		printer.Println(strings.Repeat(ch, half))
	} else {
		printer.Println(strings.Repeat(ch, tot))
	}
}

func KillIfError(ret bool, stage int, idx int) {
	if ret == false {
		job_state.mutex.Lock()
		job_state.stage = stage
		job_state.idx = idx
		// If an error occurs, notify Wait to handle error.
		xy_channel_send <- true
		// log.Printf("Notifyed")
		block := <-xy_channel_recv
		if block {
			// log.Printf("Start to block")
			xy_channel_send <- true
		}
		job_state.mutex.Unlock()
	}
}

func LogPass(msg string) {
	printer := color.New(color.FgGreen, color.Bold)
	printer.Println(msg)
}

func LogFailure(msg string) {
	printer := color.New(color.FgRed, color.Bold)
	printer.Println(msg)
}

func CleanUp() {
	RunCmd("rm -f *.gg *.gb *.ga *_err_*")
}

func TestJob(cid int) {
	chunk := xy_cnts / xy_kthreds
	for i := 0; i < chunk; i++ {
		idx := i + cid*chunk
		ret := RunCmd(fmt.Sprintf("./%s_ge %d >%d.gg 2>gen_err_%d", xy, idx, idx, idx))
		KillIfError(ret, STAGE_GEN, idx)
		ret = RunCmd(fmt.Sprintf("./%s <%d.gg >%d.ga 2>run_err_%d", xy, idx, idx, idx))
		KillIfError(ret, STAGE_RUN, idx)
		if xy_need_compare {
			ret = RunCmd(fmt.Sprintf("./%s_mp <%d.gg >%d.gb 2>cmp_err_%d", xy, idx, idx, idx))
			KillIfError(ret, STAGE_CMP, idx)
		}
		run_diff := false
		if FileExist(fmt.Sprintf("%d.ga", idx)) && FileExist(fmt.Sprintf("%d.gb", idx)) {
			run_diff = true
			ret = RunCmd(fmt.Sprintf("diff -y -W 60 %d.ga %d.gb 2>&1 1>dif_err_%d", idx, idx, idx))
			KillIfError(ret, STAGE_DIF, idx)
		}
		xy_done_mutex.Lock()
		pass := "test..." + fmt.Sprintf("%3d", idx)
		pass += strings.Repeat(" ", terminal_width-terminal_width*2/3)
		fmt.Printf(pass)
		if xy_need_compare || run_diff {
			pass = "PASS"
		} else {
			pass = "RUN"
		}
		LogPass(pass)
		xy_done++
		if xy_done == xy_cnts {
			xy_pass = true
		}
		xy_done_mutex.Unlock()
	}
}

func DumpStderr(msg string) {
	fmt.Fprintf(os.Stderr, msg)
}

func HandleError() {
	err := <-xy_channel_send
	if err {
		// Got the notification, block you guys first.
		// log.Printf("Got the notification, block you guys first")
		xy_channel_recv <- true
		// log.Printf("Handling...")
		idx := job_state.idx
		xy_done_mutex.Lock()
		if job_state.stage == STAGE_GEN {
			DrawSplit("-", fmt.Sprintf("generating error %d", idx))
			DumpStderr(string(ReadFile(fmt.Sprintf("gen_err_%d", idx))))
			DrawSplit("-", "")
		} else if job_state.stage == STAGE_RUN {
			DrawSplit("-", fmt.Sprintf("running error %d", idx))
			DumpStderr(string(ReadFile(fmt.Sprintf("run_err_%d", idx))))
			DrawSplit("-", "")
			inp := fmt.Sprintf("%d.gg", idx)
			DumpStderr(string(ReadFile(inp)))
			DrawSplit("-", "")
			data := "\n" + string(ReadFile(inp))
			WriteFile(fmt.Sprintf("%s.in", xy), []byte(data))
		} else if job_state.stage == STAGE_CMP {
			DrawSplit("-", fmt.Sprintf("comparing error %d", idx))
			DumpStderr(string(ReadFile(fmt.Sprintf("cmp_err_%d", idx))))
			DrawSplit("-", "")
			DumpStderr(string(ReadFile(fmt.Sprintf("%d.gg", idx))))
			DrawSplit("-", "")
		} else if job_state.stage == STAGE_DIF {
			DrawSplit("-", fmt.Sprintf("diffing error %d", idx))
			DumpStderr(string(ReadFile(fmt.Sprintf("dif_err_%d", idx))))
			DrawSplit("-", "input")
			inp := fmt.Sprintf("%d.gg", idx)
			DumpStderr(string(ReadFile(inp)))
			data := string(ReadFile(fmt.Sprintf("gen_err_%d", idx)))
			if len(data) > 0 {
				DrawSplit("-", "generate info")
				DumpStderr(data)
			}
			data = string(ReadFile(fmt.Sprintf("run_err_%d", idx)))
			if len(data) > 0 {
				DrawSplit("-", "running info")
				DumpStderr(data)
			}
			DrawSplit("-", "")
			data = "\nInput\n" + string(ReadFile(inp)) + "Output\n"
			data += string(ReadFile(fmt.Sprintf("%d.gb", idx)))
			WriteFile(fmt.Sprintf("%s.in", xy), []byte(data))
		} else {
			log.Fatal()
		}
		CleanUp()
		xy_kill = true
	}
}

type winsize struct {
	Row    uint16
	Col    uint16
	Xpixel uint16
	Ypixel uint16
}

func GetWidth() int {
	ws := &winsize{}
	ret, _, err := syscall.Syscall(
		syscall.SYS_IOCTL,
		uintptr(syscall.Stdin),
		uintptr(syscall.TIOCGWINSZ),
		uintptr(unsafe.Pointer(ws)))
	if int(ret) == -1 {
		panic(err)
	}
	return int(ws.Col)
}

func main() {
	if len(os.Args) == 3 {
		if os.Args[1] == "--cnt" {
			xy_cnts, _ = strconv.Atoi(os.Args[2])
			xy_cnts = xy_cnts / xy_kthreds * xy_kthreds
		}
	}

	// Get the current directory name
	cwd, _ := os.Getwd()
	tmp := strings.Split(cwd, "/")
	xy = tmp[len(tmp)-1]

	// Get the terminal width.
	terminal_width = GetWidth()

	if FileExist(xy + "_mp") {
		xy_need_compare = true
	}

	CleanUp()
	xy_channel_send = make(chan bool)
	xy_channel_recv = make(chan bool)
	// NOTE: only spwan 4 goroutines to do all the jobs.
	for i := 0; i < xy_kthreds; i++ {
		go TestJob(i)
	}
	go HandleError()
	for {
		if xy_kill {
			LogFailure("\nSOME TESTS FAILED.")
			CleanUp()
			os.Exit(1)
		}
		if xy_pass {
			LogPass("\nALL TESTS PASSED.")
			CleanUp()
			os.Exit(0)
		}
		time.Sleep(100 * time.Millisecond)
	}
}
