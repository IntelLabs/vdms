import java.io.IOException;

import java.lang.System;

import java.net.Socket;
import java.net.UnknownHostException;

import java.util.concurrent.BlockingQueue;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.List;
import java.util.ArrayList;

import java.nio.ByteBuffer;

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

    }

   public void AddToProducerQueue(VdmsTransaction message )
   {
      
      try
      {
         //Add the new message into producer queue and update the message id value
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
         message.SetId(newMessageId);
         newMessageId++;
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

   
}

