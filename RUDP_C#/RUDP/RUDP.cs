using System;
using System.Collections;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Reflection.PortableExecutable;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Xml.Serialization;

namespace RUDP
{
    public class RUDP
    {
        const int PAYLOADSIZE = 1000;
        const int MAX_QUEUE_SIZE = 10;
        const int MAX_SEND_RETRIES = 20;
        const int PACKET_RESEND_TIMEOUT = 1000;
        const int PING_PERIIOD = 1000;
        const int ENABLE_PING = 0;

        private short local_port;
        private IPAddress local_addr;
        private UdpClient client;
        IPEndPoint remote_ep;
        
        private bool incoming_num_initialized = false;
        private UInt32 incoming_data_index = 0;
        private UInt32 next_outgoing_data_index = 0;

        private long last_ping_receive_time;
        private long last_ping_send_time;

        private SessionState sessionState;
        

        public event Action<byte[]> OnBytesReceived;
        //public event Action<byte[]> OnError;
        private LinkedList<MessageFrame> packets = new LinkedList<MessageFrame>(); //treating it like a queue

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct PacketHeader
        {
            public MessageType type;
            public ushort PayloadSize;
            public ushort checksum;
            public UInt32 num;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct DataPacket
        {
            public PacketHeader header;
            public byte[] Data;

            public DataPacket(ushort payloadSize)
            {
                header.PayloadSize = payloadSize;
                Data = new byte[header.PayloadSize];
            }
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct MessageFrame
        {
            public DataPacket packet;
            public EndPoint ep;
            public byte times_sent;
            public long last_time_sent;
        }

        public enum MessageType : byte
        {
            SYN,
            ACK,
            DATA,
            FIN,
            RAW_UDP,
            PING
        }

        public enum SessionState
        {
            OPENING,
            OPEN,
            CLOSING,
            CLOSED
        }


        public RUDP(IPAddress addr, short port)
        {
            local_addr = addr;
            local_port = port;
            client = new UdpClient(new IPEndPoint(addr, local_port));
            Console.WriteLine($"{addr}:{local_port}");
            sessionState = SessionState.CLOSED;
            next_outgoing_data_index = (UInt32)new Random().Next(1, 1000);
        }

        public SessionState GetState()
        {
            return sessionState;
        }

        public bool Connect(IPAddress remote_addr, short remote_port)
        {
            if (sessionState != SessionState.CLOSED)
            {
                //Console.WriteLine("Already Connected, Invalid");
                return false;
            }
            remote_ep = new IPEndPoint(remote_addr, remote_port);
            sessionState = SessionState.OPENING;
            DataPacket packet = new DataPacket(0)
            {
                header =
                {
                    type = MessageType.SYN,
                    num = next_outgoing_data_index
                }
            };
            next_outgoing_data_index = next_outgoing_data_index + 1;
            SendPacket(packet, new IPEndPoint(remote_addr, remote_port));

            return true;
        }
        // public int Send(byte[] bytes, int offset, int byte_len)
        // {
        //     byte[] toSend = new byte[byte_len];
        //     Array.Copy(bytes, offset, toSend, 0, byte_len);
        //     return Send(toSend,byte_len);
        // }
        public int Send(byte[] bytes, int byte_len)
        {
            if(byte_len > PAYLOADSIZE)
            {
                //Console.WriteLine("size too big");
                return - 1;
            }
            if(packets.Count >= MAX_QUEUE_SIZE)
            {
                //Console.WriteLine("Queue too big");
                return -1;
            }
            DataPacket p = new DataPacket((ushort)bytes.Length)
            {
                header =
                {
                    type = MessageType.DATA,
                    num =  next_outgoing_data_index,
                },
                Data = bytes,
            };

            MessageFrame f;
    
            p.header.checksum = CalculateChecksum(p.Data);
            next_outgoing_data_index = next_outgoing_data_index + 1;
            SendPacket(p, remote_ep);

            return 0;
        }
        public int Send(byte[] bytes, int offset, int byte_len)
        {
            byte[] toSend = new byte[byte_len];
            Array.Copy(bytes, offset, toSend, 0, byte_len);
            return Send(toSend, byte_len);
        }
        private void SendPacket(DataPacket out_packet, IPEndPoint ep_)
        {
            packets.AddLast(new MessageFrame
            {
                packet = out_packet,
                last_time_sent = 0,
                ep = ep_
            });
        }

        public int PacketsWaiting()
        { return packets.Count; }

        private void SendAck(DataPacket out_packet)
        {
            Console.WriteLine($"Sending: {out_packet.header.type}:{out_packet.header.num}");
            out_packet.header.checksum = CalculateChecksum(out_packet.Data);
            ReadOnlyMemory<byte> serializedData = SerializeToSpan(out_packet);
            client.Send(serializedData.ToArray(), serializedData.Length, remote_ep);
        }

        public void Run()
        {
            ReceiveHandler();
            Sender();
        }

        public void Close()
        {
            Console.WriteLine("Closing");
            client.Close();
        }

        private void ReceiveHandler()
        {
            
            try
            {
                if (client.Client == null)
                    return;
                IPEndPoint remoteEndPoint = new IPEndPoint(IPAddress.Any, 0);
                client.Client.ReceiveTimeout = 100;
                byte[] receivedBytes = client.Receive(ref remoteEndPoint);
                DataPacket receivedPacket = DeserializeFromBytes(receivedBytes);

                ushort receivedChecksum = CalculateChecksum(receivedPacket.Data);

                if (receivedChecksum != receivedPacket.header.checksum)
                {
                    Console.WriteLine("Invalid checksum");
                    return;
                }

                Console.WriteLine($"Receive: {receivedPacket.header.type}:{receivedPacket.header.num}");
                last_ping_receive_time = millis();
                switch (receivedPacket.header.type)
                {
                    case MessageType.SYN:
                        {
                            if(sessionState != SessionState.CLOSED)
                            {
                                //Console.WriteLine("Received SYN on a open session");
                            }
                            remote_ep = remoteEndPoint;
                            sessionState = SessionState.OPEN;
                            incoming_data_index = receivedPacket.header.num;
                            //send back an ack
                            DataPacket packet = new DataPacket(0)
                            {
                                header =
                                {
                                    type = MessageType.ACK,
                                    num = incoming_data_index + 1
                                }
                            };
                            SendAck(packet);
                            break;
                        }
                    case MessageType.DATA:
                        {
                            if (sessionState != SessionState.OPEN)
                            {
                                //Console.WriteLine("Received DATA on a closed session");
                            }
                            if(!incoming_num_initialized)
                            {
                                incoming_num_initialized = true;
                                incoming_data_index = receivedPacket.header.num - 2;

                            }
                            if (incoming_data_index + 2 != receivedPacket.header.num)
                            {
                                //Console.WriteLine("Mismatch indexes, duplicate data");
                                //return;
                            }
                            else
                            {
                                OnBytesReceived?.Invoke(receivedPacket.Data);
                            }
                            //foreach (byte b in receivedPacket.Data)
                            //{
                            //    Console.Write($"0x{b:X2} ");  // Prints each byte in hexadecimal format
                            //}
                            //Console.WriteLine();
                            
                            incoming_data_index = receivedPacket.header.num;
                            //send back an ack
                            DataPacket packet = new DataPacket(0)
                            {
                                header =
                                {
                                    type = MessageType.ACK,
                                    num = incoming_data_index + 1
                                }
                            };
                            SendAck(packet);
                            break;
                        }
                    case MessageType.ACK:
                        {
                            if (packets.Count == 0)
                            {
                                //Console.WriteLine($"Um what, ack on no packets {receivedPacket.header.num}");
                                return;
                            }
                            MessageFrame thisPacket = packets.First.Value;
                            if(thisPacket.packet.header.num+1 == receivedPacket.header.num)
                            {
                                if(sessionState == SessionState.OPENING)
                                {
                                    sessionState = SessionState.OPEN;
                                }
                                packets.RemoveFirst();
                            }
                            break;
                        }
                    case MessageType.PING:
                        {
                            last_ping_receive_time = millis();
                            break;
                        }
                }
            }
            catch (Exception ex)
            {
                //Console.WriteLine($"Error receiving data: {ex.Message}");
            }
            
        }

        private void Sender()
        {
            if (ENABLE_PING==1)
            {
                if (millis() - last_ping_send_time > PING_PERIIOD && GetState() == SessionState.OPEN)
                {
                    last_ping_send_time = millis();
                    Console.WriteLine($"Sending: PING");
                    DataPacket packet = new DataPacket(0);
                    packet.header.PayloadSize = 0;
                    packet.header.num = 0;
                    packet.header.type = MessageType.PING;
                    packet.header.checksum = CalculateChecksum(packet.Data);
                    ReadOnlyMemory<byte> serializedData = SerializeToSpan(packet);
                    client.Send(serializedData.ToArray(), serializedData.Length, remote_ep);
                }

                if (millis() - last_ping_receive_time > PING_PERIIOD * 3 && GetState() == SessionState.OPEN)
                {
                    last_ping_send_time = millis();
                    Console.WriteLine("Lost Comms");
                    Close();
                    return;
                }
            }

            if (packets.Count == 0)
                return;
            
            MessageFrame thisPacket = packets.First.Value;
            
            if (thisPacket.times_sent > MAX_SEND_RETRIES)
            {
                Console.WriteLine("Sending Failed");
                sessionState = SessionState.CLOSED;
                return;
            }

            if (millis() - thisPacket.last_time_sent > PACKET_RESEND_TIMEOUT)
            {
                thisPacket.times_sent++;
                thisPacket.last_time_sent = millis();
                Console.WriteLine($"Sending: {thisPacket.packet.header.type}:{thisPacket.packet.header.num}:{thisPacket.packet.header.checksum}");
                ReadOnlyMemory<byte> serializedData = SerializeToSpan(thisPacket.packet);
                client.Send(serializedData.ToArray(), serializedData.Length, (IPEndPoint?)thisPacket.ep);
            }
            
            packets.First.Value = thisPacket;
        }

        // Correct serialization function
        public static DataPacket DeserializeFromBytes(byte[] byteArray)
        {
            // First, create an instance of PacketHeader from the byte array
            int headerSize = Marshal.SizeOf<PacketHeader>();
            byte[] headerBytes = new byte[headerSize];
            Array.Copy(byteArray, 0, headerBytes, 0, headerSize);

            // Deserialize the header part
            PacketHeader header = DeserializePacketHeader(headerBytes);

            // Then, handle the rest of the byte array as the data
            int dataSize = header.PayloadSize;
            byte[] dataBytes = new byte[dataSize];
            Array.Copy(byteArray, headerSize, dataBytes, 0, dataSize);

            // Create a DataPacket struct and assign the deserialized values
            DataPacket dataPacket = new DataPacket((ushort)dataSize)
            {
                header = header,
                Data = dataBytes
            };

            return dataPacket;
        }

        private static PacketHeader DeserializePacketHeader(byte[] headerBytes)
        {
            IntPtr ptr = Marshal.AllocHGlobal(headerBytes.Length);
            try
            {
                Marshal.Copy(headerBytes, 0, ptr, headerBytes.Length);
                return Marshal.PtrToStructure<PacketHeader>(ptr);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
        }
        public static ReadOnlyMemory<byte> SerializeToSpan(DataPacket myStruct)
        {
            // First, serialize the PacketHeader
            int headerSize = Marshal.SizeOf<PacketHeader>();
            byte[] headerBytes = new byte[headerSize];

            // Serialize the PacketHeader
            IntPtr ptr = Marshal.AllocHGlobal(headerSize);
            try
            {
                Marshal.StructureToPtr(myStruct.header, ptr, false);
                Marshal.Copy(ptr, headerBytes, 0, headerSize);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }

            // Serialize the Data array
            byte[] dataBytes = myStruct.Data;

            // Create a byte array large enough to hold both the header and data
            byte[] byteArray = new byte[headerSize + dataBytes.Length];

            // Copy the header and data into the final byte array
            Array.Copy(headerBytes, 0, byteArray, 0, headerSize);
            Array.Copy(dataBytes, 0, byteArray, headerSize, dataBytes.Length);

            // Return as ReadOnlyMemory for immutability
            return new ReadOnlyMemory<byte>(byteArray);
        }

        public static ushort CalculateChecksum(byte[] data)
        {
            if (data == null || data.Length == 0)
                return 0;

            ushort sum = 0;
            foreach (byte b in data)
            {
                
                sum += b;
            }

            return (ushort)(sum%0xFFFF);
        }


        private long millis()
        {
            long millis = DateTime.UtcNow.Ticks / TimeSpan.TicksPerMillisecond;
            return millis;
        }
    }
}
