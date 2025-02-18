using System;
using System.IO;
using System.Net;
using RUDP;

static void HandleReceivedBytes(byte[] bytes)
{
    foreach (var b in bytes)
    {
        Console.Write($"{b:X2} ");
    }
    Console.WriteLine();
}

Console.WriteLine("Starting the Client");

// Adjust these values based on needs
int send_size = 900;   // Size of each packet to send

// Read the firmware file into a byte array
string filePath = "firmware.txt";
byte[] data = File.ReadAllBytes(filePath);

int send_index = 0;

RUDP.RUDP p = new RUDP.RUDP(IPAddress.Parse("100.81.167.126"), 15680);
p.Connect(IPAddress.Parse("100.91.81.117"), 15671);
p.OnBytesReceived += HandleReceivedBytes;
// Wait for the connection to be open
while (p.GetState() != RUDP.RUDP.SessionState.OPEN)
{
    p.Run();
}
Console.WriteLine("running");
while (p.GetState() != RUDP.RUDP.SessionState.CLOSED)
{
    p.Run();

    // Send the data in chunks of 'send_size', adjusting to ensure you send the remaining data
    int remainingData = data.Length - send_index;
    int currentSendSize = Math.Min(send_size, remainingData);

    if (p.Send(data, send_index, currentSendSize) != -1)
    {
        
        send_index += currentSendSize;
        Console.WriteLine($"Sent {send_index} of {data.Length} bytes | {(((float)send_index/(float)data.Length)*100)}");
        // If all data is sent, exit the loop
        if (send_index == data.Length)
        {
            break;
        }
    }
}
while (true)
{
    p.Run();
}
