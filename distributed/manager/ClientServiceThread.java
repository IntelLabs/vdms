import java.io.IOException;
import java.io.DataInputStream;
import java.io.DataOutputStream;

import java.net.Socket;

import java.util.Arrays;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ArrayBlockingQueue;



class ClientServiceThread extends Thread
{ 
    Socket myClientSocket;
    boolean m_bRunThread = true;
    int id;
    int type;
    int messageId;
    TestServer manager;
    BlockingQueue<VdmsTransaction> responseQueue;
    
    
    public ClientServiceThread()
    { 
        super();
        responseQueue = null;
    } 
    
    ClientServiceThread(TestServer nManager, Socket s, int nThreadId) 
    {
        responseQueue = new ArrayBlockingQueue<VdmsTransaction>(128);
        manager = nManager;
        myClientSocket = s;
        id = nThreadId;
        type = -1;
        messageId = 0;
    } 
    
    public int GetType()
    {
        return type;
    }
    
    public int GetId()
    {
        return id;
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
        byte[] threadIdArray = new byte[4];
        
        int bytesRead;
        int int0;
        int int1;
        int int2;
        int int3;
        int readSize;
        int returnedThreadId;
        VdmsTransaction returnedMessage;
        VdmsTransaction newTransaction;
        
        try
        { 
            in = new DataInputStream(myClientSocket.getInputStream());
            out = new DataOutputStream(myClientSocket.getOutputStream());
            
            while(m_bRunThread) 
            {
                bytesRead = in.read(readSizeArray, 0, 4);
                int0 = readSizeArray[0] & 255;
                int1 = readSizeArray[1] & 255;
                int2 = readSizeArray[2] & 255;
                int3 = readSizeArray[3] & 255;
                readSize = int0 + (int1 << 8) + (int2 << 16) + (int3 << 24);
                //now i can read the rest of the data
                System.out.println("readsizearray - " + Arrays.toString(readSizeArray) + Integer.toString(readSize));		
                
                //if we have not determined if this node is a producer or consumer
                if(type == -1)
                {                
                    //Producer from host that are unaware that this is a node mimicking vdms
                    if(readSize > 0)
                    {
                        type = 0;
                        manager.AddNewProducer(this);
                    }
                    //Consumer that are aware this is a a node mimimicking vdms and listening for messages
                    else
                    {
                        type = 1;
                        manager.AddNewConsumer(this);
                        readSize = -1 * readSize;
                    }
                }
                
                byte[] buffer = new byte[readSize];
                bytesRead = in.read(buffer, 0, readSize);
                System.out.println("buffer - " + Arrays.toString(buffer));
                
                if(type == 0)
                {
                    //if type is producer - put the data in out queue and then wait for data in the in queue                    
                    newTransaction = new VdmsTransaction(readSizeArray, buffer, id);
                    manager.AddToConsumerQueue(newTransaction);
                    returnedMessage = responseQueue.take();
                    out.write(returnedMessage.GetSize());
                    out.write(returnedMessage.GetBuffer());
                    ++messageId;  
                }
                //if type is producer - put the data in out queue and then wait for data in the in queue
                else
                {
                    //first message from consumer nodes does not have data that should be sent to the producers
                    //bust still need to wait for a message to go into response queue before proceeding
                    if(messageId > 0)
                    {
                        bytesRead = in.read(threadIdArray, 0, 4);
                        int0 = threadIdArray[0] & 255;
                        int1 = threadIdArray[1] & 255;
                        int2 = threadIdArray[2] & 255;
                        int3 = threadIdArray[3] & 255;
                        returnedThreadId = int0 + (int1 << 8) + (int2 << 16) + (int3 << 24);
                        newTransaction = new VdmsTransaction(readSizeArray, buffer, returnedThreadId);
                        manager.AddToProducerQueue(newTransaction);
                    }
                    returnedMessage = responseQueue.take();
                    out.write(returnedMessage.GetSize());
                    out.write(returnedMessage.GetBuffer());
                    out.write(returnedMessage.GetThreadId());
                    ++messageId;
                }
                
                if(!manager.GetServerOn()) 
                { 
                    System.out.println("Server stopped");
                    out.flush(); 
                    m_bRunThread = false;
                } 
                else if(bytesRead == -1) 
                {
                    m_bRunThread = false;
                    manager.SetServerOn(false);
                } 
                else 
                {
                    out.flush(); 
                } 	
            } 
        } 
        catch(Exception e) 
        { 
            //e.printStackTrace();
            //Exception here could indicate that the socket was closed
        } 
        finally 
        { 
            try
            { 
                in.close(); 
                out.close(); 
                myClientSocket.close();
                //future - check type and remove from the approrpriate list
                System.out.println("Socket Connection Closed"); 
            }
            catch(IOException ioe)
            { 
                ioe.printStackTrace(); 
            } 
        } 
    } 
}
