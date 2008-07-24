<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
	"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html>
<head>
<title>Lamistra Online Arbiter</title>
<link rel="StyleSheet" href="arbiter.css" title="Arbiter" type="text/css" />
</head>
<body>
<h1><span class="title">Lamistra Online Arbiter</span></h1>

<p class="quote">
If you know the enemy and know yourself, your victory will not stand in doubt;<br />
if you know Heaven and know Earth, you may make your victory complete.</p>

<div class="box">
<h2>Play match</h2>
<?php
$players = array();
$dh = opendir('../players');
while (($file = readdir($dh)) !== FALSE)
{
    if(is_dir('../players/'.$file) && $file{0} != '.')
        $players[] = $file;
}
closedir($dh);
sort($players);
?>
<form method="post" action="play.php">
<p><b>Red player:</b><br />
<select name="RedPlayer"><option value=""></option><?php
foreach($players as $player)
    echo '<option value="', $player, '">', htmlentities($player), '</option>';
?></select></p>
<p><b>Blue player:</b><br />
<select name="BluePlayer"><option value=""></option><?php
foreach($players as $player)
    echo '<option value="', $player, '">', htmlentities($player), '</option>';
?></select></p>
<p><input type="submit" value="Play..." /></p>
</form>
</div>

<div class="box">
<h2>Add player program</h2>
<form method="post" enctype="multipart/form-data" action="add_player.php">
<p><strong>Program name:</strong><br />
<input type="text" name="Name" /></p>
<p><strong>Source file:</strong><br />
<input type="file" name="Source" /></p>
<p><strong>Source language:</strong><br />
<select name="Language">
<option value=""></option>
<option value="c">C (using GCC 2.95.3)</option>
<option value="c++">C++ (using GCC 2.95.3)</option>
<option value="java">Java (using Sun JDK 1.3.1)</option>
</select></p>
<p><input type="checkbox" name="DeleteSource" value="yes" checked="checked" />
Erase source file after compilation.</p>
<p><input type="submit" value="Add..." /></p>
</form>
</div>

<div class="footer">Copyright &copy; 2004 by Maks Verver
(<a href="mailto:maks@hell.student.utwente.nl">maks@hell.student.utwente.nl</a>)</div>

</body>
</html>
