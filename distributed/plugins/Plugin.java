import java.lang.System;

import java.util.ArrayList;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.List;

public class Plugin
{
   protected BlockingQueue<VdmsTransaction> producerDataQueue;
   protected BlockingQueue<VdmsTransaction> consumerDataQueue;
   protected QueueServiceThread producerService;
   protected QueueServiceThread  consumerService;
   protected List<PublisherServiceThread> producerList;
   protected List<SubscriberServiceThread> consumerList;
   protected int threadId;
   protected int newMessageId;
   protected int outgoingMessageRegistrySize;
   protected ArrayList<Integer>[] outgoingMessageRegistry;
   protected ArrayList<VdmsTransaction>[] outgoingMessageBuffer;
   
   public Plugin()
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

      //initialize the outgoign queue registry that stores information abou tmessages that have been sent
      outgoingMessageRegistrySize = 256;
      outgoingMessageRegistry = (ArrayList<Integer>[]) new ArrayList[outgoingMessageRegistrySize];
      outgoingMessageBuffer = (ArrayList<VdmsTransaction>[]) new ArrayList[outgoingMessageRegistrySize];
      for(int i = 0; i < outgoingMessageRegistrySize; i++)
      {
         outgoingMessageRegistry[i] = new ArrayList<Integer>();
         outgoingMessageBuffer[i] = new ArrayList<VdmsTransaction>();
      }

   }
   
   public void AddToProducerQueue(VdmsTransaction message )
   {
      try
      {
         //For now check to see how many
         outgoingMessageBuffer[message.GetId() % outgoingMessageRegistrySize].add(message);
         //If this is the first message recived for this outgoing message then send it back as the response
         if(outgoingMessageBuffer[message.GetId() % outgoingMessageRegistrySize].size() == 1)
         {
            producerDataQueue.put(message);
         }

         //we remove messages that are index - 1/2 (buffer size) for MessageBuffer and the MessageRegistry
         if(message.GetId() - (int) (outgoingMessageRegistrySize / 2) >=0 )
         {
            outgoingMessageBuffer[(message.GetId() - (int) (outgoingMessageRegistrySize / 2))%outgoingMessageRegistrySize].clear();
            outgoingMessageRegistry[(message.GetId() - (int) (outgoingMessageRegistrySize / 2))%outgoingMessageRegistrySize].clear();
         }
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
         message.SetId(newMessageId);
         message.SetTimestamp(System.currentTimeMillis());
         newMessageId++;
         consumerDataQueue.put(message);
      }
      catch(InterruptedException e)
      {
         e.printStackTrace();
         System.exit(-1);
      }
   } 
   
   //Add this entry to list of consumers
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

   public void AddOutgoingMessageRegistry(int messageId, int threadId)
   {
      outgoingMessageRegistry[messageId % outgoingMessageRegistrySize].add(threadId);
   }     
}