package main

import (
	"flag"
	"log"
	"net/http"
)

func main() {
	port      := flag.String("p", "80", "port to serve on")
	directory := flag.String("d", ".",  "the directory of static file to host")
	prefix    := flag.String("x", "",   "the URLs prefix")
	flag.Parse()

	http.Handle("/", http.FileServer(http.Dir(*directory)))
	if len(*prefix) != 0 {
		http.Handle(*prefix, http.StripPrefix(*prefix, http.FileServer(http.Dir(*directory))))
	}

	log.Printf("Serving %s on HTTP port: %s\n", *directory, *port)
	log.Fatal(http.ListenAndServe(":"+*port, nil))
}

