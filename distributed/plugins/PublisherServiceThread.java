import java.io.IOException;
import java.io.DataInputStream;
import java.io.DataOutputStream;

import java.net.Socket;

import java.util.Arrays;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ArrayBlockingQueue;

import java.nio.charset.StandardCharsets;

class PublisherServiceThread extends Thread
{ 
    Socket serverSocket;
    boolean m_bRunThread = true;
    int id;
    int type;
    int messageId;
    Plugin manager;
    BlockingQueue<VdmsTransaction> responseQueue;
    VdmsTransaction initSequence;
    
    public PublisherServiceThread()
    { 
        super();
        responseQueue = null;
        initSequence = null;
        
    } 
    
    PublisherServiceThread(Plugin nManager, Socket s, int nThreadId, VdmsTransaction nInitSequence) 
    {
        responseQueue = new ArrayBlockingQueue<VdmsTransaction>(128);
        manager = nManager;
        serverSocket = s;
        id = nThreadId;
        messageId = 0;
        initSequence = nInitSequence;
    } 
    
    
    public void Publish(VdmsTransaction newMessage)
    {
        responseQueue.add(newMessage);
    }
    
    public void run() 
    { 
        DataInputStream in = null; 
        DataOutputStream out = null;
        byte[] readSizeArray = new byte[4];
        
        
        int int0;
        int int1;
        int int2;
        int int3;
        int readSize;
        VdmsTransaction returnedMessage;
        Boolean threadInitFlag = false;
        VdmsTransaction newTransaction = null;
        
        try
        { 
            in = new DataInputStream(serverSocket.getInputStream());
            out = new DataOutputStream(serverSocket.getOutputStream());
            
            while(m_bRunThread) 
            {
                if(threadInitFlag == false)
                {
                    //only write the value if there is a valid init sequence
                    if(initSequence != null)
                    {
                        out.write(initSequence.GetSize());
                        out.write(initSequence.GetBuffer());
                    }
                    threadInitFlag = true;
                }
                
                in.read(readSizeArray, 0, 4);
                int0 = readSizeArray[0] & 255;
                int1 = readSizeArray[1] & 255;
                int2 = readSizeArray[2] & 255;
                int3 = readSizeArray[3] & 255;
                readSize = int0 + (int1 << 8) + (int2 << 16) + (int3 << 24);
                //now i can read the rest of the data
                System.out.println("publisher readsizearray - " + Arrays.toString(readSizeArray));		
                
                byte[] buffer = new byte[readSize];
                in.read(buffer, 0, readSize);
                //Debug code to print the bytes received in readable text
                //String tmpString = new String(buffer, StandardCharsets.UTF_8);
                //System.out.println(tmpString);
                //System.out.println("buffer - " + Arrays.toString(buffer));
                
                newTransaction = new VdmsTransaction(readSizeArray, buffer);
                //System.out.println("Server Says :" + Integer.toString(tmpInt));
                
                manager.AddToConsumerQueue(newTransaction);
                returnedMessage = responseQueue.take();
                out.write(returnedMessage.GetSize());
                out.write(returnedMessage.GetBuffer());
                ++messageId;  
                
            }
        }
        catch(Exception e) 
        { 
            e.printStackTrace(); 
        } 
        finally 
        { 
            try
            { 
                in.close(); 
                out.close(); 
                serverSocket.close(); 
                System.out.println("...Stopped"); 
            }
            catch(IOException ioe)
            { 
                ioe.printStackTrace(); 
            } 
        } 
    } 
}
