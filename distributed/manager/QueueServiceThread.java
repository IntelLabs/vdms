

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.ArrayList;
import java.util.List;

class QueueServiceThread extends Thread 
{ 
   BlockingQueue<VdmsTransaction> queue;
   TestServer manager;
   int matchType;
   
   
   public QueueServiceThread(BlockingQueue<VdmsTransaction> nQueue, TestServer nManager, int nMatchType)
   {
      queue = nQueue;
      manager = nManager;
      matchType = nMatchType;
   }
   
   public void run()
   {
      VdmsTransaction message;
      List<ClientServiceThread> publishList;
      
      try
      {
         while(true)
         {
            message = queue.take();
            
            //Get any new publishers that may exist
            if(matchType == 0)
            {
               publishList = manager.GetConsumerList();
               //Publish to all of the associated threads
               for(int i = 0; i < publishList.size(); i++)
               {
                  publishList.get(i).Publish(message);
               }
               
            }
            else
            {
               //Before sending data, get a list of any potential new Producers
               publishList = manager.GetProducerList();
               ClientServiceThread thisClient;
               for(int i = 0; i < publishList.size(); i++)
               {
                  thisClient = publishList.get(i);
//                  if(thisClient.GetId() == message.GetThreadId())
//                  {
                     thisClient.Publish(message);                     
//                  }
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
