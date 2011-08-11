#include <stdlib.h>
#include "marketSim.h"
#include "market.h"
#include "limit.h"


void *marketWorker(void *arg) {

	char none;
	
	while (1) {
		
		none = 0;
		if ( (msq->empty == 0) && (lbq->empty == 0) ) {
			if(lbq->item[lbq->head].price1 >= currentPriceX10){
				mlPairDelete( msq, lbq );
				none=1;
			}
		}

		if ((mbq->empty == 0) && (lsq->empty == 0) ) {
			if(lsq->item[lsq->head].price1 <= currentPriceX10){
				mlPairDelete( mbq, lsq );
				none=1;
			}
		}

		if ( (msq->empty == 0) && (mbq->empty == 0) && (none == 0) ) {
			mmPairDelete();
		}
	}
	return arg;
}

/* Check if both queues have at least one order available and deletes the first order from both market queues */
void mmPairDelete() {
	/* Lock both queues */
	pthread_mutex_lock(msq->mut);
	pthread_mutex_lock(mbq->mut);
	
	/*Check if a pair of market orders is available */
	if ((!msq->empty) && (!mbq->empty)) {
		
		/*
		 * Perform the transaction, print it in the logfile 
		 */
		int vol1 = msq->item[msq->head].vol;
		int vol2 = mbq->item[mbq->head].vol;
		long int id1 = msq->item[msq->head].id;
		long int id2 = mbq->item[mbq->head].id;
		order ord;
		
		if (vol1 < vol2) {
			
			vol2 = vol2 - vol1;
			queueDel(msq,&ord);
			qGetFirst(mbq)->vol = vol2;
			pthread_cond_broadcast(msq->notFull);
			
		} else if (vol1 > vol2) {
			
			vol1 = vol1 - vol2;
			queueDel(mbq,&ord);
			qGetFirst(msq)->vol = vol1;
			pthread_cond_broadcast(mbq->notFull);
			
		} else {
			
			queueDel(mbq,&ord);
			queueDel(msq,&ord);
			pthread_cond_broadcast(msq->notFull);
			pthread_cond_broadcast(mbq->notFull);
			
		}
		fprintf(log_file,"%08ld	%08ld	%5.1f	%05d	%08ld	%08ld\n", ord.timestamp, getTimestamp(), (float)currentPriceX10/10, vol1,id1,id2);
		fflush(log_file);
	}

	/* Unlock both queues and return */
	pthread_mutex_unlock(msq->mut);
	pthread_mutex_unlock(mbq->mut);

}

/*
 *  Check if both both the queue and the list are not empty. If so, and also if the rest of the requirements are met,
 *  performs the trasaction and prints it to the log file.
 */
void mlPairDelete( queue *q, queue *l ){
	/* Lock both the list, the queue and the current price mutexes */
	pthread_mutex_lock(l->mut);
	pthread_mutex_lock(q->mut);
	pthread_mutex_lock(price_mut);

	if( !q->empty && !l->empty ){
		if (((l->item[l->head].action == 'S') && (l->item[l->head].price1 <= currentPriceX10)) || ((l->item[l->head].action == 'B') && (l->item[l->head].price1 >= currentPriceX10))){
			int vol1 = q->item[q->head].vol;
			int vol2 = l->item[l->head].vol;
			int pvol = 0;
			long int id1 = q->item[q->head].id;
			long int id2 = l->item[l->head].id;
			order ord;

			if (vol1 < vol2) {
				vol2 = vol2 - vol1;
				pvol = vol1;
				queueDel(q,&ord);
				l->item[l->head].vol = vol2;
				pthread_cond_broadcast(q->notFull);
				currentPriceX10 = l->item[l->head].price1;

			} else if (vol1 > vol2) {
				vol1 = vol1 - vol2;
				pvol = vol2;
				queueDel(l,&ord);
				qGetFirst(q)->vol = vol1;
				pthread_cond_broadcast(l->notFull);
				currentPriceX10 = ord.price1;
			} else {
				pvol = vol1;
				queueDel(q,&ord);
				queueDel(l,&ord);
				pthread_cond_broadcast(q->notFull);
				pthread_cond_broadcast(l->notFull);
				currentPriceX10 = ord.price1;
			}
			fprintf(log_file,"%08ld	%08ld	%5.1f	%05d	%08ld	%08ld\n", ord.timestamp, getTimestamp(),(float) currentPriceX10/10, pvol, id1, id2);
			fflush(log_file);
		}
	}

	/* Unlock the list, the queue and the current price mutexes and return */
	pthread_mutex_unlock(price_mut);
	pthread_mutex_unlock(l->mut);
	pthread_mutex_unlock(q->mut);

}
