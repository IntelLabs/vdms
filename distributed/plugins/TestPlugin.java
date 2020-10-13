import java.io.IOException;

import java.lang.System;

import java.net.Socket;
import java.net.UnknownHostException;

import java.util.concurrent.BlockingQueue;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.List;
import java.util.ArrayList;

import java.nio.ByteBuffer;

public class TestPlugin
{

   private final BlockingQueue<VdmsTransaction> producerDataQueue;
   private final BlockingQueue<VdmsTransaction> consumerDataQueue;
   private QueueServiceThread producerService;
   private QueueServiceThread  consumerService;
   private List<PublisherServiceThread> producerList;
   private List<SubscriberServiceThread> consumerList;
   private int threadId;
   private int newMessageId;
    
   public TestPlugin()
   {
   //Create a queue for each direction 
   producerDataQueue = new ArrayBlockingQueue<VdmsTransaction>(256);
   producerService = new QueueServiceThread(producerDataQueue, this, 1);
   producerService.start();
   consumerDataQueue = new ArrayBlockingQueue<VdmsTransaction>(256);
   consumerService = new QueueServiceThread(consumerDataQueue, this, 0);
   consumerService.start();
   
   producerList = new ArrayList<PublisherServiceThread>();
   consumerList = new ArrayList<SubscriberServiceThread>();
   threadId = 0;
   newMessageId = 0;

   byte[] initSize = new byte[] {(byte)0xfc, (byte)0xff, (byte)0xff, (byte)0xff};
   byte[] initPayload = ByteBuffer.allocate(4).putInt(0).array();

   VdmsTransaction initSequence = new VdmsTransaction(initSize, initPayload);

      String sourceNodes = System.getenv("SOURCES");
      String sourceNodesArray[] = sourceNodes.split(",");
      int sourceNodeCount = sourceNodesArray.length;
      String destinationNodes = System.getenv("DESTINATIONS");
      String destinationNodesArray[] = destinationNodes.split(",");
      int destinationNodeCount = destinationNodesArray.length;

      for(int i = 0; i < sourceNodeCount; i++)
     {
         String connectionInfo[] = sourceNodesArray[i].split(":");
         int connectionPort = Integer.parseInt(connectionInfo[1]);
         System.out.println(connectionInfo[0] + " " + Integer.toString(connectionPort));
         try 
         {
            Socket thisSocket = new Socket(connectionInfo[0], connectionPort);
            PublisherServiceThread thisServerServivceThread = new PublisherServiceThread(this, thisSocket, threadId, initSequence);
            AddNewProducer(thisServerServivceThread);
         }
         catch (UnknownHostException e)
        {
           e.printStackTrace();
        }
         catch (IOException e)
        {
           e.printStackTrace();
        }
     }
      
      for(int i = 0; i < destinationNodeCount; i++)
     {
         String connectionInfo[] = destinationNodesArray[i].split(":");
         int connectionPort = Integer.parseInt(connectionInfo[1]);
         System.out.println(connectionInfo[0] + " " + Integer.toString(connectionPort));
         try 
        {
            Socket thisSocket = new Socket(connectionInfo[0], connectionPort);
            SubscriberServiceThread thisServerServivceThread = new SubscriberServiceThread(this, thisSocket, threadId, null);
            AddNewConsumer(thisServerServivceThread);
         }
         catch (UnknownHostException ex)
        {
            System.out.println("Server not found: " + ex.getMessage());
        }
         catch (IOException ex)
        {
            System.out.println("I/O error: " + ex.getMessage());
        }
     }
      
     //Start all of the threads. We start the destination nodes first so that no data from the sources is acceptd
     for(int i = 0; i < producerList.size(); i++)
     {
        producerList.get(i).start();
     }
     for(int i = 0; i < consumerList.size(); i++)
     {
        consumerList.get(i).start();
     }



    }

   public void AddToProducerQueue(VdmsTransaction message )
   {
      
      try
      {
         //Add the new message into producer queue and update the message id value
         message.SetId(newMessageId);
         newMessageId++;
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


   public void AddNewConsumer(SubscriberServiceThread nThread)
   {
      consumerList.add(nThread);
   }


   public void AddNewProducer(PublisherServiceThread nThread)
   {
      producerList.add(nThread);
   }

   public List<SubscriberServiceThread> GetConsumerList()
   {      
      return consumerList;
   }

   public List<PublisherServiceThread> GetProducerList()
   {      
      return producerList;
   }

   public static void main (String[] args)
   {
      new TestPlugin();
   }

}

