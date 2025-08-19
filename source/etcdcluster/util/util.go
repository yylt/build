package util

import (
	"fmt"
	"os"
	"strconv"
)

// Helper functions
func FileExists(path string) bool {
	_, err := os.Stat(path)
	return !os.IsNotExist(err)
}

func DirExists(path string) bool {
	info, err := os.Stat(path)
	if os.IsNotExist(err) {
		return false
	}
	return info.IsDir()
}

func MustAtoi(s string) int {
	i, err := strconv.Atoi(s)
	if err != nil {
		panic(fmt.Sprintf("failed to convert string to int: %s", s))
	}
	return i
}

func Min(a, b int) int {
	if a < b {
		return a
	}
	return b
}
