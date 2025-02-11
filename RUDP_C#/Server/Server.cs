using System;
using System.IO;
using System.Net;
using RUDP;

// Server
Console.WriteLine("Starting the Server");


const int send_size = 10;
byte[] data = new byte[100];
int send_index = 0;
for (int i = 0; i < data.Length; i++)
{
    data[i] = (byte)(i/send_size);
}

RUDP.RUDP p = new RUDP.RUDP(IPAddress.Loopback, 15671);
string filePath = "firmware.txt";

using (FileStream fs = new FileStream(filePath, FileMode.Create, FileAccess.Write)){}

void HandleReceivedBytes(byte[] bytes)
{
    // Append the received bytes to the file "firmware.bin"
    string filePath = "firmware.txt";

    // Use FileStream with append mode to write bytes to the file
    using (FileStream fs = new FileStream(filePath, FileMode.Append, FileAccess.Write))
    {
        fs.Write(bytes, 0, bytes.Length);
    }
    
    //Console.WriteLine("Bytes received and saved:");
    //foreach (var b in bytes)
    //{
    //    Console.Write($"{b:X2} ");
    //}
    //Console.WriteLine();
}

p.OnBytesReceived += HandleReceivedBytes;

while (true)
{
    int remainingData = data.Length - send_index;
    int currentSendSize = Math.Min(send_size, remainingData);

    if (p.GetState() == RUDP.RUDP.SessionState.OPEN && remainingData > 0 && p.Send(data, send_index, currentSendSize) != -1)
    {
        send_index += currentSendSize;
        Console.WriteLine($"Sent {send_index} of {data.Length} bytes | {(((float)send_index / (float)data.Length) * 100)}");
        // If all data is sent, exit the loop
        if (send_index == data.Length)
        {

        }
    }
    p.Run();
}
