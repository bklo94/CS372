Readme

COMPILE INSTRUCTIONS
chatclient.c with "gcc chatclient.c -o chatclient"

USAGE
1.Start server first (note the server you are running on i.e. flip1,flip2,flip3...)
"python chatserve [PORT NUMBER]"
i.e.
"python chatserve 2005"

2.Once started a "Waiting for Response" message should appear

3.Start the client
Use the same PORT NUMBER as the server and the HOSTNAME that the python program was started on
"./chatclient" [HOSTNAME] [PORT NUMBER]"
i.e.
"./chatclient" flip1 2005"

4.Once started the client will prompt for a handle, if the handle is longer than 10 characters it will truncate it to 10

5.If prompted for input on the client, then connection has been establish. Also the server will confirm the connection established with a message of "Connection established with [IP Address]"

6.Quitting on the client with "\quit" shows a "Closed Connection" message and the program exits

7.Quitting on the server with "\quit" shhows a "Connection Closed, Now Waiting For A Response" Message and the client shows a "Connection closed by server\nClosed Connection" Message 



