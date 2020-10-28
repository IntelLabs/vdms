

import java.util.concurrent.BlockingQueue;
import java.util.List;

class QueueServiceThread extends Thread 
{ 
   BlockingQueue<VdmsTransaction> queue;
   Plugin manager;
   int matchType;
   
   
   public QueueServiceThread(BlockingQueue<VdmsTransaction> nQueue, Plugin nManager, int nMatchType)
   {
      queue = nQueue;
      manager = nManager;
      matchType = nMatchType;
   }
   
   public void run()
   {
      VdmsTransaction message;
      List<PublisherServiceThread> publishList;
      List<SubscriberServiceThread> subscribeList;
      
      try
      {
         while(true)
         {
            message = queue.take();
            
            //Get any new publishers that may exist
            if(matchType == 0)
            {
               subscribeList = manager.GetConsumerList();
               //Publish to all of the associated threads
               for(int i = 0; i < subscribeList.size(); i++)
               {
                  subscribeList.get(i).Publish(message);
               }
               
            }
            else
            {
               //Before sending data, get a list of any potential new Producers
               publishList = manager.GetProducerList();
               //Publish to all of the associated threads
               for(int i = 0; i < publishList.size(); i++)
               {
                  publishList.get(i).Publish(message);
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
