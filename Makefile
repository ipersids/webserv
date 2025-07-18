run:
    ulimit -n 100000 && ./webserv //default ulimit is 1000 we need to cover the max value for connections here

