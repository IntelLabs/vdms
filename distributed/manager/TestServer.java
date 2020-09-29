import java.io.BufferedReader;
import java.io.IOException;
import java.io.DataInputStream;
import java.io.DataOutputStream;

import java.lang.System;

import java.nio.ByteBuffer;

import java.net.ServerSocket;
import java.net.Socket;
import java.net.UnknownHostException;
import java.text.SimpleDateFormat;

import java.util.concurrent.BlockingQueue;

import jdk.internal.joptsimple.internal.Messages;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.Calendar;
import java.util.List;
import java.util.Arrays;
import java.util.ArrayList;


public class TestServer {
   ServerSocket myServerSocket;
   private final BlockingQueue producerQueue;
   private final BlockingQueue consumerQueue;
   private QueueServiceThread producerService;
   private QueueServiceThread  consumerService;
   List<Integer> threadTypeArray;
   List<ClientServiceThread> threadArray;
   int newThreadId;
   
   boolean ServerOn = true;

        
    public TestServer() { 
      //Create a queue for each direction 
      threadTypeArray = new ArrayList();
      producerQueue = new ArrayBlockingQueue<VdmsTransaction>(256);
      producerService = new QueueServiceThread(producerQueue, threadArray, 0);
      producerService.start();
      consumerQueue = new ArrayBlockingQueue<VdmsTransaction>(256);
      consumerService = new QueueServiceThread(consumerQueue, threadArray, 1);
      consumerService.start();
      threadArray = new ArrayList();
      newThreadId = 0;


      try 
      {
	      myServerSocket = new ServerSocket(Integer.parseInt(System.getenv("NETWORK_PORT")));
      } 
      catch(IOException ioe) 
      { 
         System.out.println("Could not create server socket");
         System.exit(-1);
      } 
		
      Calendar now = Calendar.getInstance();
      SimpleDateFormat formatter = new SimpleDateFormat(
         "E yyyy.MM.dd 'at' hh:mm:ss a zzz");
      System.out.println("It is now : " + formatter.format(now.getTime()));
      
      while(ServerOn) { 
         try { 
            Socket clientSocket = myServerSocket.accept();
            ClientServiceThread cliThread = new ClientServiceThread(this, clientSocket, newThreadId);
            cliThread.start();
            newThreadId++;
            threadTypeArray.add(-1);
            threadArray.add(cliThread);
         } 
         catch(IOException ioe)
         { 
            System.out.println("Exception found on accept. Ignoring. Stack Trace :"); 
            ioe.printStackTrace(); 
         }  
      } 
      try { 
            myServerSocket.close(); 
            System.out.println("Server Stopped"); 
         } 
      catch(Exception ioe) { 
         System.out.println("Error Found stopping server socket"); 
         System.exit(-1); 
      } 
   }

   public void SetThreadType(int threadId, int threadType )
   {
      threadTypeArray.set(threadId, threadType);
   }

   public void AddToProducerQueue(VdmsTransaction message )
   {
      try
      {
         producerQueue.put(message);
      }
      catch(InterruptedException e)
      {
         e.printStackTrace();
         System.exit(-1);
      }
   }

   public void AddToConsumerQueue(VdmsTransaction message )
   {
      try
      {
         consumerQueue.put(message);
      }
      catch(InterruptedException e)
      {
         e.printStackTrace();
         System.exit(-1);
      }
   }

   public BlockingQueue<VdmsTransaction> GetProducerQueue()
   {
      return producerQueue;
   }

   public BlockingQueue<VdmsTransaction> GetConsumerQueue()
   {
      return consumerQueue;
   }

   public static void main (String[] args) {

       
       
      new TestServer();
   }


















   class QueueServiceThread extends Thread 
   { 
      BlockingQueue<VdmsTransaction> queue;
      List<Integer> threadTypeArray;
      int matchType;

      public QueueServiceThread(BlockingQueue<VdmsTransaction> nQueue, List<ClientServiceThread> nThreadArray, int nMatchType)
      {
         queue = nQueue;
         threadArray = nThreadArray;
         matchType = nMatchType; 
      }

      public void run()
      {
         VdmsTransaction message;
         try
         {
            while(true)
            {
               message = queue.take();
               for(int i = 0; i < threadTypeArray.size(); i++)
               {
                  if(threadArray.get(i).GetType() == matchType)
                  {
//                     threadArray.get(i).Publish(message);
                  }
               }
            }

         }
         catch(InterruptedException e)
         {
            this.interrupt();
            
         }


      }

   }













   class ClientServiceThread extends Thread { 
      Socket myClientSocket;
      boolean m_bRunThread = true;
      int id;
      int type;
      int messageId;
      TestServer manager;
      BlockingQueue<VdmsTransaction> responseQueue;
      BlockingQueue<VdmsTransaction> outputQueue;


      public ClientServiceThread() { 
         super();
         responseQueue = null;
      } 
		
      ClientServiceThread(TestServer nManager, Socket s, int nThreadId) 
      {
         responseQueue = new BlockingQueue<VdmsTransaction>(128);
         outputQueue = null;
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


      public void run() 
      { 
         DataInputStream in = null; 
         DataOutputStream out = null;
         byte[] readSizeArray = new byte[4];

         int bytesRead;
         int int0;
         int int1;
         int int2;
         int int3;
         int readSize;
         VdmsTransaction returnedMessage;

	 
         System.out.println(
            "Accepted Client Address - " + myClientSocket.getInetAddress().getHostName());
         try 
         { 
	         in = new DataInputStream(myClientSocket.getInputStream());
	         out = new DataOutputStream(myClientSocket.getOutputStream());
            
            while(m_bRunThread) 
            {
               bytesRead = in.read(readSizeArray, 0, 4);
               int0 = readSizeArray[0];
               int1 = readSizeArray[1];
               int2 = readSizeArray[2];
               int3 = readSizeArray[3];
               readSize = int0 + (int1 << 8) + (int2 << 16) + (int3 << 24);
               //now i can read the rest of the data
               System.out.println("readsizearray - " + Arrays.toString(readSizeArray));		

               //if we have not determined if this node is a producer or consumer
               if(type == -1)
               {                
                  //Producer from host that are unaware that this is a node mimicking vdms
                  if(readSize > 0)
                  {
                     type = 0;
                     manager.SetThreadType(id, type);
                  }
                  //Consumer that are aware this is a a node mimimicking vdms and listening for messages
                  else
                  {
                     type = 1;
                     manager.SetThreadType(id, type);
                     readSize = -1 * readSize;
                  }
               }
               


               byte[] buffer = new byte[readSize];
               bytesRead = in.read(buffer, 0, readSize);
               System.out.println("buffer - " + Arrays.toString(buffer));

               VdmsTransaction newTransaction = new VdmsTransaction(readSizeArray, buffer);
               //System.out.println("Client Says :" + Integer.toString(tmpInt));

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
                  if(messageId == 0)
                  {
                     manager.AddToProducerQueue(newTransaction);
                     returnedMessage = responseQueue.take();
                     out.write(returnedMessage.GetSize());
                     out.write(returnedMessage.GetBuffer());
                  }
                  ++messageId;
               }
               

               //if type is a consumer then wait for data in the 


               if(!ServerOn) 
               { 
                  System.out.print("Server has already stopped"); 
                  //out.println("Server has already stopped"); 
                  out.flush(); 
                  m_bRunThread = false;
               } 
		
               if(bytesRead == -1) 
               {
		            m_bRunThread = false;
                  System.out.print("Stopping client thread for client : ");
               } 
               else if(bytesRead == -1) 
               {
                  m_bRunThread = false;
                  System.out.print("Stopping client thread for client : ");
                  ServerOn = false;
               } 
               else 
               {
		            //out.println("Server Says : " + bytesRead);
                  out.flush(); 
               } 
            } 
         } catch(Exception e) 
         { 
            e.printStackTrace(); 
         } 
         finally { 
            try { 
               in.close(); 
               out.close(); 
               myClientSocket.close(); 
               System.out.println("...Stopped"); 
            } catch(IOException ioe) { 
               ioe.printStackTrace(); 
            } 
         } 
      } 
   }



}

