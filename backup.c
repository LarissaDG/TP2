 if(recv(s, &sms, sizeof(sms), MSG_PEEK)){
	        	printf("To aki\n");
	        	if((count = recv(s, &sms, sizeof(sms), 0)) < 0){
		            logexit("receive ack");
		        } 	
		        if(sms.msg_id == 5){
		        	printf("Receve FIM\n");
		        	return;
		        }
	        }