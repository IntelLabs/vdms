import java.io.IOException;
import java.io.DataInputStream;
import java.io.DataOutputStream;

import java.net.Socket;

import java.util.Arrays;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ArrayBlockingQueue;



class ServerServiceThread extends Thread
{ 
    Socket serverSocket;
    boolean m_bRunThread = true;
    int id;
    int type;
    int messageId;
    TestPlugin manager;
    BlockingQueue<VdmsTransaction> responseQueue;
    VdmsTransaction initSequence;
    
    public ServerServiceThread()
    { 
	super();
	responseQueue = null;
	initSequence = null;

    } 
    
    ServerServiceThread(TestPlugin nManager, Socket s, int nThreadId, int nType, VdmsTransaction nInitSequence) 
    {
	responseQueue = new ArrayBlockingQueue<VdmsTransaction>(128);
	manager = nManager;
	serverSocket = s;
	id = nThreadId;
	type = nType;
	messageId = 0;
	initSequence = nInitSequence;
    } 
    
    public int GetType()
    {
	return type;
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
	
	try
	    { 
		in = new DataInputStream(serverSocket.getInputStream());
		out = new DataOutputStream(serverSocket.getOutputStream());
		
		while(m_bRunThread) 
		    {
			if(threadInitFlag == false)
			{
				out.write(initSequence.GetSize());
				out.write(initSequence.GetBuffer());				
				threadInitFlag = true;
			}


			in.read(readSizeArray, 0, 4);
			int0 = readSizeArray[0] & 255;
			int1 = readSizeArray[1] & 255;
			int2 = readSizeArray[2] & 255;
			int3 = readSizeArray[3] & 255;
			readSize = int0 + (int1 << 8) + (int2 << 16) + (int3 << 24);
			//now i can read the rest of the data
			System.out.println("readsizearray - " + Arrays.toString(readSizeArray));		
			
			
			byte[] buffer = new byte[readSize];
			in.read(buffer, 0, readSize);
			System.out.println("buffer - " + Arrays.toString(buffer));
			
			VdmsTransaction newTransaction = new VdmsTransaction(readSizeArray, buffer);
			//System.out.println("Server Says :" + Integer.toString(tmpInt));
			
			if(type == 0)
			    {
				//if type is producer - put the data in out queue and then wait for data in the in queue
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
					manager.AddToProducerQueue(newTransaction);
				    }
				returnedMessage = responseQueue.take();
				out.write(returnedMessage.GetSize());
				out.write(returnedMessage.GetBuffer());
				
				++messageId;
			    }
			
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
