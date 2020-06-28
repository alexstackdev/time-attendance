const PING_INTERVAL_MILLIS = 5000;
var output = document.getElementById('output');
var socket;
var isRunning=false;
var autoSocket;
function connect(host) {
	console.log('connect', host);
	if (window.WebSocket) {
		output.value += "Connecting" + "\n";
		document.getElementById('socketStatus').innerHTML = "Trạng thái Socket:  Connecting";
		if(socket) {
			socket.close();
			socket = null;
		}
		host = "ws://" + host + ":81/ws";
		socket = new WebSocket(host);

		socket.onmessage = function(event){
			output.value += event.data+"\n"; // + "\r\n";
			output.scrollTop = output.scrollHeight;
			let dataIn= String(event.data);
			if(dataIn[0]=="!"){
				if(dataIn[1]=="2"){
					dataIn = dataIn.substring(2);
					document.getElementById('idCardToAdd').value = dataIn;
					return;
				}
				if(dataIn[1]=="1"){
					dataIn = dataIn.substring(2);
					let dTemp = new Date();
					let d = new Date((dataIn * 1000));
					console.log(d);
					document.getElementById('gio').value = d.getUTCHours();
					document.getElementById('phut').value =  d.getUTCMinutes();
					document.getElementById('giay').value = d.getUTCSeconds();
					document.getElementById('ngay').value = d.getUTCDate();
					document.getElementById('thang').value = d.getUTCMonth()+1;
					document.getElementById('nam').value = d.getUTCFullYear();
					return;
				}
				if(dataIn[1]=="4"){ //show list employees to window/tab Nhân viên 
					showListEmployess(dataIn);
					return;
				}
				if(dataIn[1]=="6") showInfoHomeTab(dataIn);  //Show Info of MCU to home tab
				
				if(dataIn[1]=="8"&&dataIn[2]=="0") {  //not found record for this id
					console.log("Nothing to show,!!! no record for this id");
					output.value += "Nothing to show,!!! no record for this id" + "\n";
					document.getElementById("containerTableSalaryByMonth").style.display = "none";
					document.getElementById("nothingToShow").style.display = "inline";
					return;
				}
				if(dataIn[1]=="8"&&dataIn[2]!="0"){
					document.getElementById("containerTableSalaryByMonth").style.display = "inline";
					document.getElementById("nothingToShow").style.display = "none";
					
					let id = dataIn[2].toString() + dataIn[3].toString() + dataIn[4].toString();
					
					dataIn = dataIn.substring(11, dataIn.length);
					showSalaryByMonth(id, 1, dataIn);
					return;
					
				}
				if(dataIn[1]=="."){ //show list employees to window/tab Nhân viên 
					let passRight = dataIn.substring(2);
					console.log("Pass web control is: "+passRight);
					var pass = prompt("Nhập mật khẩu quản trị. bấm huỷ bỏ nếu chỉ xem", "");
					if(pass!=passRight) alert("Sai mật khẩu, bạn chỉ được phép xem!!!!"); 
					else  {
					alert("Mật khẩu đúng, chào quản trị viên ^^^!!!!"); 
					/*document.getElementById("output").style.display = "inline";
					document.getElementById("formNV").style.display = "inline";
					document.getElementById("formXoaThe").style.display = "inline";
					document.getElementById("settings").style.display = "inline";*/
					document.getElementById("output").style = "width:100%;height:500px;";
					document.getElementById("formNV").style = " ";
					document.getElementById("formXoaThe").style = "";
					document.getElementById("settings").style = "";
					document.getElementById("passWebESP").value = passRight;
					}
					return;
				}
				if(dataIn[1]=="9"){
					dataIn = dataIn.substring(2);
					console.log(dataIn);
					loadWifiESP(dataIn);
					return;
				}
				
			}
		};
		
		socket.onopen = function(event) {
			isRunning = true;
			document.getElementById('socketStatus').innerHTML = "Trạng thái Socket:  Connected";
			
			output.value += "Connected" + "\n";
			output.scrollTop = output.scrollHeight;
			send("?6");
			send("?.");
			
			
			
		};
		socket.onclose = function(event) {
			isRunning = false;
			document.getElementById('socketStatus').innerHTML = "Trạng thái Socket:  Disconnected";
			output.value += "Disconnected" + "\n";
	            //socket.removeAllListeners();
	            socket = null;
				//if(!isRunning) trySocket();
		};
		socket.onerror = function(event) {
			document.getElementById('socketStatus').innerHTML = "Trạng thái Socket:  Error";
			output.value += "Error" + "\n";
		  
	            //socket.removeAllListeners();
	           socket = null;
				//if(!isRunning) trySocket();
		};
	} else {
		alert("Your browser does not support Web Socket.");
	}
}
function send(data) {
	if (!window.WebSocket) {
		return;
	}
	if (socket.readyState == WebSocket.OPEN) {
		var message = data;
		output.value += 'sending : ' + data + '\r\n';
		output.scrollTop = output.scrollHeight;
		socket.send(message);
	} else {
		//alert("The socket is not open.");
	}
}
function clearText() {
	output.value="";
}


function buttonUpdate(){
	window.location= window.location.hostname + "/update";
	
}
function trySocket(){
	output.value += "Auto try connect socket to " +window.location.hostname+ "\n";
	//connect("192.168.5.101");
	connect(window.location.hostname);
	
}

function settingFunc(){
	send("?1");
	send("?9");
}

window.onload = function(){
	
	trySocket();
	
	document.getElementById("output").style.display = "none";
	document.getElementById("formNV").style.display = "none";
	document.getElementById("formXoaThe").style.display = "none";
	document.getElementById("settings").style.display = "none";
	
	
	
	
}


function buttonUpdateWeb(){
	let pass = document.getElementById('passWebESP').value;
	if(isNaN(pass)||pass>996992||pass<100000){
		
		alert("Mật khẩu phải là số từ 100000 đến 996992, ví dụ: 123456 !!");
		return;
	}
	console.log(pass);
	send("#."+pass);
}

function loadWifiESP(dataIn){
	
	document.getElementById("nameWifiESP").value = dataIn.substring(0,dataIn.search("!"));
	dataIn = dataIn.substring(dataIn.search("!")+1, dataIn.length);
	document.getElementById("passWifiESP").value = dataIn.substring(0,dataIn.search("!"));
	dataIn = dataIn.substring(dataIn.search("!")+1, dataIn.length);
	document.getElementById("timeWifiESP").value = dataIn;
}

function buttonUpdateWifi(){
	let name = document.getElementById("nameWifiESP").value;
	let pass = document.getElementById("passWifiESP").value;
	let timeout = document.getElementById("timeWifiESP").value;
	console.log(name);
	console.log(pass);
	console.log(timeout);
	if(isNaN(pass)||pass<10000000||pass > 9999999999999){
		alert("Mật khẩu nên là số từ 10000000 đến 9999999999999.   ví dụ: 123456789 hoặc 0328081234 !!");
		return;
	}
	if(isNaN(timeout)||timeout>3600||timeout<300){
		alert("timeout phải là số từ 300 đến 3600, ví dụ: 600 !!");
		return;
	}
	
	name = name.replace(/[_\W]+/g, "");
	if(name==""||name.length>30||name.length<6){
		alert("Tên wifi không được để trống, tên phải từ 3 đến 30 kí tự !!");
		return;
	}

	send("#9"+name+"!"+pass+"!"+timeout);
	console.log("#9"+name+"!"+pass+"!"+timeout);
	alert("Ngắt -> Cấp nguồn lại để thay đổi có hiệu lực !!");
	return;
	
}

function buttonAddMem(){
	console.log("f");
	let id = document.getElementById('idNewToAdd');
	let name = document.getElementById('nameNewToAdd');
	let idCard = document.getElementById('idCardToAdd');
	
	if(id.value==""){
		if(isNaN(idCard.value)){
		alert("ID card phải là số, ví dụ: 23435467245 !!");
		return;
		}
		if(idCard.value==""){
		alert("ID card không được để trống");
		return;
		}
		if(name.value=="")send("#3000"+ idCard.value);
		else send("#3000"+ idCard.value +"!"+name.value);
		console.log("#3000"+ idCard.value +"!"+name.value);
		id.value ="";
		name.value ="";
		idCard.value ="";
		var x = document.getElementById("tableEmployess").rows.length-1;
		if(x!=0) for( x; x>0 ; x--)	document.getElementById("tableEmployess").deleteRow(x);
		tab2Event();
		return;
	}
	
	console.log(id.value);
	if(id.value<100||id.value>999||isNaN(id.value)){
	alert("ID không hợp lệ, ID hợp lệ từ 100 đến 999 !!");
	return;
	}	
	if(isNaN(idCard.value)){
	alert("ID card phải là số, ví dụ: 23435467245 !!");
	return;
	}
	if(idCard.value==""){
	alert("ID card không được để trống");
	return;
	}
	
	
	if(name.value==""){
		send("#3"+id.value+ idCard.value+"!");
		console.log("#3"+id.value+ idCard.value+"!");
	}
	else{ 
		send("#3"+id.value+ idCard.value +"!"+name.value);
		console.log("#3"+id.value+ idCard.value +"!"+name.value);
	}
	
	
	
	
	id.value ="";
	name.value ="";
	idCard.value ="";
	var x = document.getElementById("tableEmployess").rows.length-1;
	if(x!=0) for( x; x>0 ; x--)	document.getElementById("tableEmployess").deleteRow(x);
	tab2Event();
	return;
	
	
	
	
}

function useTimePC(){
  let d = new Date();
	document.getElementById('gio').value = d.getHours();
	document.getElementById('phut').value =  d.getMinutes();
	document.getElementById('giay').value = d.getSeconds();
	document.getElementById('ngay').value = d.getDate();
	document.getElementById('thang').value = d.getMonth()+1;
	document.getElementById('nam').value = d.getFullYear();
}

function buttonUpdateTime(){
	let gio = document.getElementById('gio').value;
	let phut = document.getElementById('phut').value;
	let giay = document.getElementById('giay').value;
	let ngay = document.getElementById('ngay').value;
	let thang = document.getElementById('thang').value-1;
	let nam = document.getElementById('nam').value;
	if(gio==""||gio==""||gio==""||gio==""||gio==""||gio==""||isNaN(gio)||isNaN(phut)||isNaN(giay)||isNaN(ngay)||isNaN(thang)||isNaN(nam)) {
		alert("thời gian không hợp lệ, kiểm tra lại!!!");
		return;
	}
	let d = new Date(nam, thang, ngay, gio, phut, giay, 0);
	console.log(d.getTime());
	if(d.getTime()<946731601000||d.getTime()>4133980799000){
		alert("Thời gian không hợp lệ, thời gian cho phép từ 0:0:1 1/1/2000 đến 23:59:59 31/12/2100");
		return;
	}
	
	d = new Date(d.getTime() - (d.getTimezoneOffset()*60000));
	console.log(d);
	send("#1"+(d.getTime()/1000));
	
}


function tab2Event(){
	send("?4");
	
}

function showListEmployess(dataIn){
	var x = document.getElementById("tableEmployess").rows.length-1;
	if(x!=0) for( x; x>0 ; x--)	document.getElementById("tableEmployess").deleteRow(x);
	var table = document.getElementById("tableEmployess");
	var listByMonth = document.getElementById("listNhanVienTheoThang");
	
	
	console.log(dataIn);
	console.log(dataIn.length);
	

	dataIn = dataIn.substring(2);
	let s1 = dataIn.search("#");
	let idCount="";
	for( i = 0; i<s1; i++){
		idCount += dataIn[i];
	}
	idCount= parseInt(idCount);
	if(idCount==0) {
		document.getElementById("nameList").innerHTML = "Không có nhân viên";
		return;
	}
	document.getElementById("nameList").innerHTML = "Tổng số nhân viên: " + idCount.toString();
	
	dataIn = dataIn.substring(s1+1,dataIn.length);
	console.log(dataIn);
	
	//new type by json^^
	let obj = JSON.parse(dataIn);
	let z =1;
	let first=0;
	for(i in obj){
		let row = table.insertRow(z);
		let idString="";
		row.insertCell(0).innerHTML = i;
		console.log(i);
		for(j in obj[i]){
			if(j == "name"){
				row.insertCell(1).innerHTML = obj[i][j];
				table.rows[z].onclick = function () {
				console.log(this);
				changeName(this);
				};
				if(first==0) {
					listByMonth.innerHTML = "";
					first =1;
				}
				listByMonth.innerHTML += "<li><a onclick='showSalaryByMonth(" +i+")'>" +obj[i][j]+ " ,ID: " + i+ "</a></li>";
				listByMonth.innerHTML += "<li class='divider'></li>";
			}			
			if(j != "name"){
				idString += obj[i][j];
				idString += ",";
			}				
		}
		if(idString[idString.length-1]==",") idString = idString.substring(0,idString.length-1);
		row.insertCell(2).innerHTML = idString;
		z++;
	}
}
document.getElementById("btnDeleteCardID").addEventListener("click", deleteCardId);
function deleteCardId(){
	let id = document.getElementById("idCardToDelete").value;
	if(isNaN(id)){
		alert(id + "không phải id hợp lệ, id hợp lệ phải là số");
		return;
	}
	console.log("deleting " + id + "from all DB");
	send("#5"+id);
	alert("OKKKK");
	send("?4");
	id = "";
	return;
}
function showInfoHomeTab(dataIn){

	dataIn = dataIn.substring(3, dataIn.length);
	console.log(dataIn);
	let s1 = dataIn.search("#");
	document.getElementById("chipID").innerHTML = "Chip ID: " + dataIn.substring(0,s1);
	dataIn = dataIn.substring(s1+1, dataIn.length);
	console.log(dataIn);
	
	s1 = dataIn.search("#");
	document.getElementById("sketchSize").innerHTML = "Sketch sử dụng: " + dataIn.substring(0,s1) + " byte";
	dataIn = dataIn.substring(s1+1, dataIn.length);
	console.log(dataIn);
	
	s1 = dataIn.search("#");
	document.getElementById("heapFree").innerHTML = "Heap còn trống: " + dataIn.substring(0,s1) + " byte";
	dataIn = dataIn.substring(s1+1, dataIn.length);
	console.log(dataIn);
	
	s1 = dataIn.search("#");
	document.getElementById("freeSketchSize").innerHTML = "Bộ nhớ sketch còn trống:  " + dataIn.substring(0,s1) + " byte";
	dataIn = dataIn.substring(s1+1, dataIn.length);
	console.log(dataIn);
	
	s1 = dataIn.search("#");
	document.getElementById("coreVersion").innerHTML = "Chip ID: " + dataIn.substring(0,s1);
	dataIn = dataIn.substring(s1+1, dataIn.length);
	console.log(dataIn);
		
	s1 = dataIn.search("#");
	document.getElementById("MACadr").innerHTML = "Địa chỉ MAC: " + dataIn.substring(0,s1);
	dataIn = dataIn.substring(s1+1, dataIn.length);
	console.log(dataIn);
		
	s1 = dataIn.search("#");
	document.getElementById("IPAPadr").innerHTML = "Địa chỉ IP host:  " + dataIn.substring(0,s1);
	dataIn = dataIn.substring(s1+1, dataIn.length);
	console.log(dataIn);
			
	
	document.getElementById("IPLocaladr").innerHTML = "Địa chỉ IP local: " + dataIn;

	
	
}
function homeFunc(){
	send("?6");
}
function tab3Event(){
	send("?4");
	document.getElementById("containerTableSalaryByMonth").style.display = "none";
	document.getElementById("nothingToShow").style.display = "none";
	let datee = new Date();
	document.getElementById("monthGetSalary").value = datee.getMonth()+1;
	document.getElementById("yearGetSalary").value = datee.getFullYear();
	
}
function changeName(inData){
	txt = inData.innerHTML;
	let s1 = txt.search(">");
	txt = txt.substring(s1+1, txt.length);
	let id = txt.substr(0, 3);
	
	s1 = txt.search(">"); txt = txt.substring(s1+1, txt.length);
	s1 = txt.search(">"); txt = txt.substring(s1+1, txt.length);
	s1 = txt.search("<"); let name = txt.substr(0, s1);
	var newName = prompt("Nhập tên mới cho nhân viên " + id + ":", name);
	if(newName!=null){
		if(id>999||id<100){
			alert("id không hợp lệ, id phải từ 100 đến 999");
			return;
		}
		
		if(name==newName){
			alert("tên không thay đổi!!!");
			return;
		}
		send("#7"+id+newName);
		send("?4");		
	}		
}
function showSalaryByMonth(id, handle =0, dataIn=""){
	if(!handle){
		let monthGetSalary = parseInt(document.getElementById("monthGetSalary").value);
		if(monthGetSalary<10||monthGetSalary>0) monthGetSalary = "0" + monthGetSalary;
		let yearGetSalary = document.getElementById("yearGetSalary").value;

		console.log(monthGetSalary);
		console.log(yearGetSalary);
		console.log(id);
		if(id>999||id<100){
			alert("id không hợp lệ, id phải từ 100 đến 999");
			return;
		}
		send("?8"+id+monthGetSalary+yearGetSalary);
	} else{
		
		console.log(dataIn);
		var table = document.getElementById("tableSalaryByMonth");
		let countLine=0;

		dataIn = dataIn.replace("\n", ''); 
		for(let i =0; i<999; i++){
			
			if(dataIn[i]=="!") {
				console.log(i);
				countLine++;
			}
		}
		console.log(countLine);
		console.log(dataIn);
		let tableRowCount=1;
		let inWork=0;
		let lastTime=0, lastDay=0;
		let line =1;let row;let datee;
		let arrTime = [];
		for(i=0; i<countLine; i++){
			let s = dataIn.search("!");
			arrTime[i]=parseInt(dataIn.slice(0, s))*1000;
			dataIn = dataIn.substring(s+1,dataIn.length);
		}
		
		
		for(i=0; i<arrTime.length; i++){
			console.log(arrTime[i]);
		}
		
		var x = document.getElementById("tableSalaryByMonth").rows.length-1;
		if(x!=0) for( x; x>0 ; x--)	document.getElementById("tableSalaryByMonth").deleteRow(x);
		
		row = table.insertRow(tableRowCount);
		let timeStart=0;
		let dateEnd = new Date();
		let totalTimeWork = 0;
		let salaryBonus = document.getElementById("salaryBonus").value;
		for(i=0; i<arrTime.length; i++){
			datee = new Date(parseInt(arrTime[i]));
			dateEnd.setUTCDate(lastDay);
			dateEnd.setUTCMonth(datee.getUTCMonth());
			dateEnd.setUTCFullYear(datee.getUTCFullYear());
			dateEnd.setUTCHours(23);
			dateEnd.setUTCMinutes(59);
			dateEnd.setUTCSeconds(59);
			if(datee.getUTCDate()!=lastDay){
				if(inWork){
					row.insertCell(2).innerHTML = "<b>23:59:59</b>";
					inWork=0;
					row.insertCell(3).innerHTML = ((dateEnd.getTime()-timeStart)/3600000).toFixed(3);
					totalTimeWork += parseFloat(((dateEnd.getTime()-timeStart)/3600000).toFixed(3));
					row.insertCell(4).innerHTML = salaryBonus + "đ";
					row.insertCell(5).innerHTML = ((dateEnd.getTime()-timeStart)/3600000).toFixed(3) * salaryBonus;
				}
				lastDay=datee.getUTCDate();
				inWork=1;
				tableRowCount++;
				row = table.insertRow(tableRowCount);
				row.insertCell(0).innerHTML = datee.getUTCDate();
				row.insertCell(1).innerHTML = datee.getUTCHours()  + ":" + datee.getUTCMinutes()  +":"+  datee.getUTCSeconds();	
				timeStart = datee.getTime();
				continue;
			} else {
				if(inWork){
					row.insertCell(2).innerHTML = datee.getUTCHours()  + ":" + datee.getUTCMinutes()  +":"+  datee.getUTCSeconds();	
					inWork=0;
					row.insertCell(3).innerHTML = ((datee.getTime()-timeStart)/3600000).toFixed(3);
					totalTimeWork += parseFloat(((datee.getTime()-timeStart)/3600000).toFixed(3));
					row.insertCell(4).innerHTML = salaryBonus+"đ";
					row.insertCell(5).innerHTML = ((datee.getTime()-timeStart)/3600000).toFixed(3) * salaryBonus;
					continue;
				} else {
					inWork=1;
					lastDay = datee.getUTCDate();
					tableRowCount++;
					row = table.insertRow(tableRowCount);
					row.insertCell(0).innerHTML = datee.getUTCDate();
					row.insertCell(1).innerHTML = datee.getUTCHours()  + ":" + datee.getUTCMinutes()  +":"+  datee.getUTCSeconds();	
					timeStart = datee.getTime();
					continue;
				}
				
				
			}
			
		}
		if(inWork){
			row.insertCell(2).innerHTML =  "<b>23:59:59</b>";	
			row.insertCell(3).innerHTML = ((dateEnd.getTime()-timeStart)/3600000).toFixed(3);	
			totalTimeWork += parseFloat(((dateEnd.getTime()-timeStart)/3600000).toFixed(3));			
			row.insertCell(4).innerHTML = salaryBonus+"đ";
			row.insertCell(5).innerHTML = ((dateEnd.getTime()-timeStart)/3600000).toFixed(3) * salaryBonus;
		}
		// show total value 
		row = table.insertRow(tableRowCount+1);
		row.insertCell(0).innerHTML = "<b>Tổng</b>";
		row.insertCell(1).innerHTML = "N/A";
		row.insertCell(2).innerHTML = "N/A";
		row.insertCell(3).innerHTML = totalTimeWork.toFixed(3).toString() + " Giờ";
		row.insertCell(4).innerHTML = "N/A";
		row.insertCell(5).innerHTML = parseInt(totalTimeWork*salaryBonus) + "đ";
		
	
	}
	
}