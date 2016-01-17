
var arrayOfRolloverClasses = new Array();
var arrayOfClickClasses = new Array();
var activeRow = false;
var activeRowClickArray = new Array();

function highlightTableRow(){
    var tableObj = this.parentNode;
    if(tableObj.tagName!='TABLE')tableObj = tableObj.parentNode;

    if(this!=activeRow){
	this.setAttribute('origCl',this.className);
	this.origCl = this.className;
    }
    if (this.origCl == 'b_wrap'){ this.className = arrayOfRolloverClasses[tableObj.id] + ' b_wrap'; }
    else                        { this.className = arrayOfRolloverClasses[tableObj.id]; }
    activeRow = this;
}

function clickOnTableRow(){
    var tableObj = this.parentNode;
    if(tableObj.tagName!='TABLE')tableObj = tableObj.parentNode;
    
    if(activeRowClickArray[tableObj.id] && this!=activeRowClickArray[tableObj.id]){
	activeRowClickArray[tableObj.id].className='';
    }
    if (this.origCl == 'b_wrap'){ this.className = arrayOfClickClasses[tableObj.id]; + ' b_wrap'; }
    else                        { this.className = arrayOfClickClasses[tableObj.id]; }
    
    activeRowClickArray[tableObj.id] = this;
}

function resetRowStyle()  {
    var tableObj = this.parentNode;
    if(tableObj.tagName!='TABLE')tableObj = tableObj.parentNode;
    
    if(activeRowClickArray[tableObj.id] && this==activeRowClickArray[tableObj.id]){
	this.className = arrayOfClickClasses[tableObj.id];
	return;
    }
    
    var origCl = this.getAttribute('origCl');
    if(!origCl)origCl = this.origCl;
    this.className=origCl;
}

function addTableRolloverEffect(tableId,classOnOver,classOnClick)  {
    arrayOfRolloverClasses[tableId] = classOnOver;
    arrayOfClickClasses[tableId] = classOnClick;
    
    var tableObj = document.getElementById(tableId);
    var tBody = tableObj.getElementsByTagName('TBODY');

    if(tBody){
	for(var no=0;no < tBody.length; no++){
	    var rows = tBody[no].getElementsByTagName('TR');
	    addTableRolloverEffectExec(rows,classOnClick);
	}
    }else{
	var rows = tableObj.getElementsByTagName('TR');
	addTableRolloverEffectExec(rows,classOnClick);
    }
    /*
    for(var no=0;no < rows.length; no++){
	rows[no].onmouseover = highlightTableRow;
	rows[no].onmouseout  = resetRowStyle;
	
	if(classOnClick){
	    rows[no].onclick = clickOnTableRow;
	}
    }
    */
}

function addTableRolloverEffectExec(rows,classOnClick){
    for(var no=0;no < rows.length; no++){
	rows[no].onmouseover = highlightTableRow;
	rows[no].onmouseout  = resetRowStyle;
	
	if(classOnClick){
	    rows[no].onclick = clickOnTableRow;
	}
    }
}
