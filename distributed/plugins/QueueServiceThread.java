

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.ArrayList;
import java.util.List;

   class QueueServiceThread extends Thread 
   { 
      BlockingQueue<VdmsTransaction> queue;
      TestPlugin manager;
      int matchType;


      public QueueServiceThread(BlockingQueue<VdmsTransaction> nQueue, TestPlugin nManager, int nMatchType)
      {
         queue = nQueue;
         manager = nManager;
         matchType = nMatchType;
      }

      public void run()
      {
         VdmsTransaction message;
         List<ServerServiceThread> publishList;

         try
         {
            while(true)
            {
               message = queue.take();

               //Get any new publishers that may exist
               if(matchType == 0)
               {
                  publishList = manager.GetConsumerList(); 
               }
               else
               {
                  //Before sending data, get a list of any potential new Producers
                  publishList = manager.GetProducerList();
               }

               //Publish to all of the associated threads
               for(int i = 0; i < publishList.size(); i++)
               {
                  publishList.get(i).Publish(message);
               }
            }

         }
         catch(InterruptedException e)
         {
            this.interrupt();
            
         }


      }

   }
