# Web Proxy
> A web proxy in C

# Web Proxy in C

This is a simple web proxy implemented in C. The proxy acts as an intermediary between the client and the server, allowing the client to access web pages through the proxy server.

## Usage

To use the web proxy, first compile the `main.c` file using a C compiler

gcc main.c -o main


Then, start the proxy server by running the following command:

./main <port>

Replace `<port>` with the port number on which you want to run the proxy server.

Once the proxy server is running, you can configure your web browser to use the proxy server. In Firefox, for example, go to Preferences -> General -> Network Proxy and enter the IP address and port number of the proxy server.

## Features

This web proxy supports the following features:

- HTTP/1.0 and HTTP/1.1 protocols
- GET and POST requests
- Caching of web pages
- Blacklisting of URLs

## Future Improvements

There are several improvements that could be made to this web proxy, including:

- Support for HTTPS connections
- Better error handling
- More efficient caching algorithm
- Whitelisting of URLs

## License

This web proxy is released under the MIT License. 


