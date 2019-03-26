var cPage=window.location.toString().replace(/.*\//,'');
cPage=cPage.replace(/\?.*/,'');

var mNum=0;	

function Page(g,s,p,t,f)
{
    this.g=g;
	this.s=s;
	this.p=p;
	this.t=t;
	this.idx=mNum++;
	if (f) this.f=f; else this.f=0;
}

var Pages=new Array(

 new Page(0,'mode selection','opmode.html', 'Mode Selection')
,new Page(0,'ap setting','ap.html','AP Interface Setting')
,new Page(0,'sta setting','sta_config.html','STA Interface Setting')
,new Page(0,'app setting','app_config.html',' Application Setting')
,new Page(0,'device management ','management.html','Device Management')
,new Page(0,'device constellation ','constellation.html','Device constellation')

);

function findPage()
{
for (var i=0;i<Pages.length;i++)
	if (Pages[i].p==cPage) return Pages[i];
return Pages[0];
}


//<tr width="750"><td colspan=2><img border=0 src=images/img04.png ></td></tr>

function pageHead()
{
	var tmp=document.body.clientWidth ;

	m='<table><tr >';
	m=m+'<!--<td></td>-->\
	<td align="center" >\
	<table align=center WIDTH="550" cellpadding=0 cellspacing=0 >\
	  <tr>';
	m+='<td valign=top WIDTH="550" >\
	<TABLE WIDTH=550 CELLPADDING=0 CELLSPACING=0 ALIGN=center>\
	<TR BGCOLOR=white>\
		<td width=11 HEIGHT=600></td>\
		<td width=550 valign=top align=left>';
	document.writeln(m);
}

function pageTail(msg)
{
	//var p=findPage();

var m='</td>\
    <TD WIDTH=19></TD>\
</tr>\
</table>\
  </TD>\
</TR>\
\
</TR>\
</table></td>\
<!--<td></td>--></tr>\
</table>';
	document.writeln(m);
}
