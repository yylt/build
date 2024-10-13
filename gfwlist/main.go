package main

import (
	"bufio"
	"bytes"
	"encoding/base64"
	"flag"
	"fmt"
	"io"
	"net"
	"net/http"
	"os"
	"regexp"
	"strings"
	"sync"
	"time"
)

var (
	wg          = sync.WaitGroup{}
	timeout     = time.Second * 2
	tlds, fpath string
	mydomain    string

	domainCh = make(chan string, 1024)
	blackCh  = make(chan string, 1024)

	re = regexp.MustCompile(`^[a-zA-Z0-9.\|]+$`)
)

func removeSecondDot(s string) string {
	// 查找第一个 . 的位置
	firstDotIndex := strings.Index(s, ".")
	if firstDotIndex == -1 {
		// 如果没有找到 .，直接返回原字符串
		return s
	}

	// 查找第二个 . 的位置
	secondDotIndex := strings.Index(s[firstDotIndex+1:], ".")
	if secondDotIndex == -1 {
		return s
	}
	secondDotIndex += firstDotIndex + 1
	return s[firstDotIndex+1:]
}

func download() {
	url := "https://gitlab.com/gfwlist/gfwlist/raw/master/gfwlist.txt"

	// 下载文件
	resp, err := http.Get(url)
	if err != nil {
		fmt.Printf("读取 %s 失败: %s\n", url, err)
		return
	}
	defer resp.Body.Close()
	if resp.StatusCode != 200 {
		fmt.Printf("读取 %s 失败,code: %d\n", url, resp.StatusCode)
		return
	}
	// 读取文件内容
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		fmt.Printf("读取 %s 失败: %s\n", url, err)
		return
	}

	// Base64 解码
	decoded, err := base64.StdEncoding.DecodeString(string(body))
	if err != nil {
		fmt.Println("Base64 解码失败:", err)
		return
	}
	// tld
	var tld []string
	tldSlice := strings.Split(tlds, ",")
	for _, v := range tldSlice {
		tld = append(tld, strings.TrimSpace(v))
	}

	// 逐行
	scanner := bufio.NewScanner(strings.NewReader(string(decoded)))
	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())

		if !strings.HasPrefix(line, "||") {
			continue
		}
		// 检查行是否符合正则表达式
		if !re.MatchString(line) {
			continue
		}
		var include bool = false
		for _, v := range tld {
			if strings.HasSuffix(line, v) {
				include = true
				break
			}
		}
		if len(tld) == 0 {
			include = true
		}
		if !include {
			continue
		}
		wg.Add(1)
		domainCh <- strings.TrimLeft(line, "||")
	}
	close(domainCh)
}

func update() {
	var (
		i    int
		buf  bytes.Buffer
		keys = map[string]struct{}{}
	)
	buf.WriteString(`
[AutoProxy 0.2.9]
! Expires: 72h
! Title: GFW_Black_list by yylt

`)

	for b := range blackCh {
		b := removeSecondDot(b)
		if _, ok := keys[b]; ok {
			continue
		}
		keys[b] = struct{}{}
		buf.WriteString(fmt.Sprintf("||%s\n", b))
		i++
	}
	err := os.WriteFile(fpath, buf.Bytes(), 0644)
	if err != nil {
		fmt.Printf("写入文件 %s 错误: %s", fpath, err)
	}
	fmt.Println("总共数量:", i)
}

func main() {
	num := flag.Int("num", 100, "number of goroutine")
	flag.StringVar(&tlds, "tld", "com,io", "tld domain")
	flag.StringVar(&fpath, "file", "./gfwblack", "file path")
	flag.StringVar(&mydomain, "include", "google.com", "custom black domain")
	flag.Parse()

	for i := 0; i < *num; i++ {
		// 使用执行任务
		go func() {
			for domain := range domainCh {
				conn, err := net.DialTimeout("tcp", domain+":80", timeout)
				if err == nil {
					blackCh <- domain
					conn.Close()
				}
				wg.Done()
			}
		}()
	}
	download()
	go func() {
		wg.Wait()
		close(blackCh)
	}()
	update()
}
