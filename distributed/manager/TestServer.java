import java.io.IOException;

import java.lang.System;

import java.net.ServerSocket;
import java.net.Socket;
import java.net.UnknownHostException;
import java.text.SimpleDateFormat;

import java.util.concurrent.BlockingQueue;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.Calendar;
import java.util.List;
import java.util.Arrays;
import java.util.ArrayList;

import VDMS.protobufs.QueryMessage;
import com.google.protobuf.InvalidProtocolBufferException;

public class TestServer 
{
   ServerSocket myServerSocket;
   private final BlockingQueue<VdmsTransaction> producerDataQueue;
   private final BlockingQueue<VdmsTransaction> consumerDataQueue;
   private QueueServiceThread producerService;
   private QueueServiceThread  consumerService;
   List<Integer> threadTypeArray;
   List<ClientServiceThread> threadArray;
   int newThreadId;

   private List<ClientServiceThread> producerList;
   private List<ClientServiceThread> consumerList;
   
   
   boolean ServerOn;

        
    public TestServer() { 
      //Create a queue for each direction 
      threadTypeArray = new ArrayList();
      producerDataQueue = new ArrayBlockingQueue<VdmsTransaction>(256);
      producerService = new QueueServiceThread(producerDataQueue, this, 1);
      producerService.start();
      consumerDataQueue = new ArrayBlockingQueue<VdmsTransaction>(256);
      consumerService = new QueueServiceThread(consumerDataQueue, this, 0);
      consumerService.start();
      threadArray = new ArrayList();
      newThreadId = 0;

      producerList = new ArrayList();
      consumerList = new ArrayList();
      

      QueryMessage.queryMessage tmpMessage = QueryMessage.queryMessage.newBuilder().setJson("{\'a\' : 5}").build();

      byte[] tmp = new byte[] {(byte)0xff, (byte)0xff};
      try
	  {
	      QueryMessage.queryMessage newTmpMessage = QueryMessage.queryMessage.parseFrom(tmp);
	  }
      catch(InvalidProtocolBufferException e)
	  {
	      System.exit(-1);
	  }

      
      System.out.println(System.getenv("NETWORK_PORT"));
      try 
      {
	      myServerSocket = new ServerSocket(Integer.parseInt(System.getenv("NETWORK_PORT")));
         ServerOn = true;
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

   public Boolean GetServerOn()
   {
      return ServerOn;
   }

   public void SetServerOn(Boolean nValue)
   {
      ServerOn = nValue;
   }

   public void SetThreadType(int threadId, int threadType )
   {
      if(threadType == 0)
      {

      }
      else
      {

      }

      threadTypeArray.set(threadId, threadType);
   }

   public void AddToProducerQueue(VdmsTransaction message )
   {
      try
      {
         producerDataQueue.put(message);
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
         consumerDataQueue.put(message);
      }
      catch(InterruptedException e)
      {
         e.printStackTrace();
         System.exit(-1);
      }
   }


   public void AddNewConsumer(ClientServiceThread nThread)
   {
      consumerList.add(nThread);
   }


   public void AddNewProducer(ClientServiceThread nThread)
   {
      producerList.add(nThread);
   }

   public List<ClientServiceThread> GetConsumerList()
   {      
      return consumerList;
   }

   public List<ClientServiceThread> GetProducerList()
   {      
      return producerList;
   }
  public List<Integer> GetThreadTypeArray()
  {
     return threadTypeArray;
  }


   public List<ClientServiceThread> GetThreadArray()
   { 
      return threadArray;
   }


   public static void main (String[] args)
    {
      new TestServer();
   }

}

