<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
	"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html>
<head>
<title>Lamistra Online Arbiter</title>
<base href="http://<?php echo $_SERVER['SERVER_NAME'], $_SERVER['PHP_SELF']; ?>" />
<link rel="StyleSheet" href="arbiter.css" title="Arbiter" type="text/css" />
</head>
<body>
<h1><span class="title">Lamistra Online Arbiter</span></h1>

<?php
$red  = $_REQUEST['RedPlayer'];
$blue = $_REQUEST['BluePlayer'];

if(empty($red) || empty($blue))
{
?><h3>Players incorrectly specified!</h3><?php
}
else
{
    $red_dir  = '../players/'.basename($red);
    $blue_dir = '../players/'.basename($blue);
    
    $command1 = is_file($red_dir.'/run') ?
                '../execute_player '.escapeshellarg($red) :
                '/usr/local/jdk1.3.1/bin/java -cp '.escapeshellarg($red_dir).' '.escapeshellarg($red);
                
    $command2 = is_file($blue_dir.'/run') ?
                '../execute_player '.escapeshellarg($blue) :
                '/usr/local/jdk1.3.1/bin/java -cp '.escapeshellarg($blue_dir).' '.escapeshellarg($blue);
                
    $command =  sprintf( '/usr/local/bin/python ../arbiter/arbiter.py %s %s 2>&1',
                         escapeshellarg($command1), escapeshellarg($command2) );
?>
<table cellpadding="0" cellspacing="0" border="0"><tr>
<td valign="top">
    <table cellpadding="0" cellspacing="0" style="border: 2px solid black">
    <caption>Unassigned Roles</caption>
    <?php
    $classes = array(
        'B' => 'Bomb', 'G' => 'General', 'O' => 'Colonel', 'L' => 'Lieutenant',
        'M' => 'Miner', 'R' => 'Rider', 'S' => 'Spy', 'V' => 'Flag' );
    foreach($classes as $symbol => $name)
    {
    ?><tr>
    <td><div class="tile"><img src="img/R<?php echo $symbol; ?>.png"
        alt="Red <?php echo $name; ?>" title="Red <?php echo $name; ?>" /></div></td>
    <td style="vertical-align: middle; width: 30px; text-align: center; border-right: 1px solid black"><span
        id="R<?php echo $symbol; ?>_left">&nbsp;</span></td>
    <td style="vertical-align: middle; width: 30px; text-align: center; border-left: 1px solid black"><span
        id="B<?php echo $symbol; ?>_left">&nbsp;</span></td>
    <td><div class="tile"><img src="img/B<?php echo $symbol; ?>.png"
        alt="Blue <?php echo $name; ?>" title="Blue <?php echo $name; ?>" /></div></td>
    </tr><?php
    }
    ?>
    </table>
</td>

<td style="width: 10px">&nbsp;</td>

<td valign="top" style="width: 260px" align="center">
    <table cellpadding="0" cellspacing="0">
    <caption>Battlefield</caption>
    <?php
    for($row = 6; $row >= 0; --$row)
    {
        echo '<tr>';
        echo '<th style="text-align: right; vertical-align: center;">&nbsp;', 1 + $row, '&nbsp;</th>';
        for($col = 0; $col < 7; ++$col)
        {
            $title = chr(ord('a') + $col) . ($row + 1);
            echo '<td><div class="tile"><img id="field_', $row, $col, '" src="img/E.png" alt="',
                 $title, '" title="', $title ,'" /></div></td>';
        }
        echo '</tr>';
    }
    echo '<tr><td></td>';
    for($c = 'a'; $c <= 'g'; ++$c)
        echo '<th style="text-align:center; vertical-align: top">', $c, '</th>';
    echo '</tr>';
    ?>
    </table>
</td>

<td style="width: 10px">&nbsp;</td>
    
<td valign="top">
    <table cellpadding="0" cellspacing="0" style="border: 2px solid black">
    <caption>History</caption>
    <tr><td><select class="code" id="history" size="20" style="width: 150px; border: 0">
    <option></option>
    </select></td></tr>
    </table>
</td>
</tr></table>

<table cellpadding="0" cellspacing="0" border="0"><tr>
<td colspan="5" style="text-align:center">
    <br />
    <table cellpadding="0" cellspacing="0">
    <tr><td>
        <table cellpadding="0" cellspacing="0" style="border: 2px solid black">
        <caption>Comments Red</caption>
        <tr><td><textarea id="R_comments" rows="8" cols="40" style="font-size: 11px"></textarea></td></tr>
        </table>
    </td>
    <td style="width: 10px">&nbsp;</td>
    <td>
        <table cellpadding="0" cellspacing="0" style="border: 2px solid black">
        <caption>Comments Blue</caption>
        <tr><td><textarea id="B_comments" rows="8" cols="40" style="font-size: 11px"></textarea></td></tr>
        </table>
    </td></tr>
    </table>
</td>
</tr></table>

<div class="box" style="margin-top: 2em">
<h2>
<span style="color: red"><?php echo htmlentities($red); ?></span>
<span style="color: black">vs</span>
<span style="color: blue"><?php echo htmlentities($blue); ?></span>
</h2>
<p id="messages"></p>
</div>

<script type="text/javascript" src="play.js"></script>
<?php
}
?>

<p><a href="index.php">Back to homepage.</a></p>

<div class="footer">Copyright &copy; 2004 by Maks Verver
(<a href="mailto:maks@hell.student.utwente.nl">maks@hell.student.utwente.nl</a>)</div>

<?php
if(isset($command))
{
    flush();
    $fp = popen($command, 'r');
    $comments = array();
    while(($line = fgets($fp)) !== FALSE)
    {
        echo '<script type="text/javascript">';
        
        $line = rtrim($line);
        $matches = array();
        if(preg_match('/^([a-g])([1-7])-([a-g])([1-7]) ([BGOLMRSV-]) ([BGOLMRSV-])/', $line, $matches))
        {
            echo sprintf( 'execute("%s", %d, %d, %d, %d, %s, %s);',
                          $matches[0],
                          ord($matches[2]) - ord('1'), ord($matches[1]) - ord('a'), 
                          ord($matches[4]) - ord('1'), ord($matches[3]) - ord('a'),
                          ($matches[5] == '-') ? 'null' : '"'.$matches[5].'"',
                          ($matches[6] == '-') ? 'null' : '"'.$matches[6].'"' );
                          
            foreach($comments as $player => $lines)
                echo sprintf('comment("%s", "%s");', $player, implode('\n', $lines));
            $comments = array();
        }
        else
        if(preg_match('/^ *(\\w+)> ?(.*)$/', $line, $matches))
        {
            $comments[$matches[1]][] = $matches[2];
        }
        else
        {
            echo sprintf('message("%s");', addslashes($line));
        }
        
        echo '</script>', "\n";
        flush();
    }


    echo '<script type="text/javascript">';
    foreach($comments as $player => $lines)
        echo sprintf('comment("%s", "%s");', $player, implode('\n', $lines));
    echo '</script>', "\n";

    fclose($fp);
}
?>

</body>

</html>
