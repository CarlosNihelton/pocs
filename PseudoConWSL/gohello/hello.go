package main

import (
	"context"
	"fmt"

	"github.com/ubuntu/gowsl"
)

func main() {
	ctx := context.Background()
	d := gowsl.NewDistro(ctx, "UbuntuWork")
	// d.Shell(gowsl.WithCommand("ping -c 8 8.8.8.8"))
	cmd := d.Command(ctx, "ping -c 4 8.8.8.8")
	out, err := cmd.CombinedOutput()
	if err == nil {
		fmt.Println(string(out))
	}
}

/*

package main

import (
	"fmt"
	"io"
	"log"
	"os"
	"sync"
	"syscall"
	"unsafe"

	"golang.org/x/sys/windows"
)

var (
	// WSL api.
	wslAPIDll    = syscall.NewLazyDLL("wslapi.dll")
	apiWslLaunch = wslAPIDll.NewProc("WslLaunch")
)

func main() {
	distroName, err := syscall.UTF16PtrFromString("Ubuntu-Preview")
	if err != nil {
		log.Fatal("could not convert distro name to UTF16")
	}

	command, err := syscall.UTF16PtrFromString("ping -c 8 8.8.8.8")
	if err != nil {
		log.Fatal("could not convert command to UTF16")
	}

	const useCWD = 1
	var handle windows.Handle

	outR, outW, err := os.Pipe()
	if err != nil {
		log.Fatalf("Could not create pipe: %v", err)
	}

	var wg sync.WaitGroup

	wg.Add(1)
	go func() {
		defer wg.Done()
		if _, err := io.Copy(os.Stdout, outR); err != nil {
			log.Fatalf("Err copying: %v", err)
		}
	}()

	inR, inW, err := os.Pipe()
	if err != nil {
		log.Fatalf("Could not create pipe: %v", err)
	}

	devNull, err := os.Open(os.DevNull)
	if err != nil {
		log.Fatalf("Could not open dev null: %v", err)
	}
	defer devNull.Close()

	wg.Add(1)
	go func() {
		defer wg.Done()
		if _, err := io.Copy(inW, devNull); err != nil {
			log.Fatalf("Err copying: %v", err)
		}
	}()

	ret, _, err := apiWslLaunch.Call(
		uintptr(unsafe.Pointer(distroName)),
		uintptr(unsafe.Pointer(command)),
		uintptr(useCWD),
		inR.Fd(),
		outW.Fd(),
		outW.Fd(),
		uintptr(unsafe.Pointer(&handle)))
	if ret != 0 {
		log.Fatalf("Failed syscall (%d): %v", ret, err)
	}

	///// Close after start
	inR.Close()
	outW.Close()
	////

	pid, err := windows.GetProcessId(handle)
	if err != nil {
		log.Fatalf("failed to find launched process: %v", err)
	}

	p, err := os.FindProcess(int(pid))
	if err != nil {
		log.Fatalf("Could not find process: %v", err)
	}

	state, err := p.Wait()
	if err != nil {
		log.Fatalf("Could not wait process: %v", err)
	}

	///// Close after wait
	outR.Close()
	inW.Close()
	////

	wg.Wait()

	fmt.Printf("Exit code: %d", state.ExitCode())
}

*/
