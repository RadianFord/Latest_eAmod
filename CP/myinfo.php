<?php
	session_start();
	include_once 'config.php';
	include_once 'functions.php';

	if (!isset($_SESSION[$CONFIG['Name'] . 'member_id']) || $_SESSION[$CONFIG['Name'] . 'member_id'] <= 0)
		redir("news.php", "main_div", "Debes estar logeado con tu cuenta de Miembro para accesar aca");

	if (isset($_POST['pais']) && strlen($_POST['pais']) == 2 && strcmp($_POST['pais'],".."))
	{
		$member_id = $_SESSION[$CONFIG['Name'] . 'member_id'];
		$pais = $_POST['pais'];

		$_SESSION[$CONFIG['Name'] . 'pais'] = $pais;
		$mysql->query("UPDATE `members` SET `pais` = '$pais' WHERE `member_id` = '$member_id'", $CONFIG['DBMain']);
	}

	opentable("Membresia Evangelis Ragnarok");
?>
	<table width="550">
		<tr>
			<td align="center" height="30" valign="middle">
				Bienvenido a Evangelis Ragnarok, esperamos que estes pasando un buen tiempo<br>
				Para mantener un control de membres&iacute;s y su informaci&oacute;n es importante que mantengas actualizados tus datos.<br>
				Utiliza esta secci&oacute;n para actualizar tu informaci&oacute;n.<br>
			</td>
		</tr>
		<tr><td align="center" height="10" valign="top">&nbsp;</td></tr>
		<tr>
			<td align="center" height="10" valign="top">
				<b>Opci&oacute;n actual de Pa&iacute;s :</b> <?php echo (file_exists("./images/flags/" . strtolower($_SESSION[$CONFIG['Name'] . 'pais']) . ".png") ? '<img src="./images/flags/' . strtolower($_SESSION[$CONFIG['Name'] . 'pais']) . '.png"> ':' ') . $_SESSION[$CONFIG['Name'] . 'pais']; ?>
			</td>
		</tr>
		<tr><td align="center" height="10" valign="top">&nbsp;</td></tr>
		<tr>
			<td align="center" height="10" valign="top">
				<b>Pa&iacute;s donde Vives :</b>&nbsp;
				<form id="selectpais">
					<select name="pais" onchange="javascript:POST_ajax('myinfo.php','main_div','selectpais');">
						<option value=".." selected>Selecciona tu nuevo Pais</option>
						<option value="AF">Afganist&aacute;n</option>
						<option value="AL">Albania</option>
						<option value="DE">Alemania</option>
						<option value="AD">Andorra</option>
						<option value="AO">Angola</option>
						<option value="AI">Anguila</option>
						<option value="AQ">Ant&aacute;rtida</option>
						<option value="AG">Antigua y Barbuda</option>
						<option value="AN">Antillas Neerlandesas</option>
						<option value="SA">Arabia Saud&iacute;</option>
						<option value="DZ">Argelia</option>
						<option value="AR">Argentina</option>
						<option value="AM">Armenia</option>
						<option value="AW">Aruba</option>
						<option value="AU">Australia</option>
						<option value="AT">Austria</option>
						<option value="AZ">Azerbaiy&aacute;n</option>
						<option value="BS">Bahamas</option>
						<option value="BH">Bahr&aacute;in</option>
						<option value="BD">Bangladesh</option>
						<option value="BB">Barbados</option>
						<option value="BE">B&eacute;lgica</option>
						<option value="BZ">Belice</option>
						<option value="BJ">Ben&iacute;n</option>
						<option value="BM">Bermudas</option>
						<option value="BY">Bielorrusia</option>
						<option value="BO">Bolivia</option>
						<option value="BA">Bosnia y Hercegovina</option>
						<option value="BW">Botsuana</option>
						<option value="BR">Brasil</option>
						<option value="BN">Brun&eacute;i</option>
						<option value="BG">Bulgaria</option>
						<option value="BF">Burkina Faso</option>
						<option value="BI">Burundi</option>
						<option value="BT">But&aacute;n</option>
						<option value="CV">Cabo Verde</option>
						<option value="KH">Camboya</option>
						<option value="CM">Camer&uacute;n</option>
						<option value="CA">Canad&aacute;</option>
						<option value="TD">Chad</option>
						<option value="CL">Chile</option>
						<option value="CN">China</option>
						<option value="CY">Chipre</option>
						<option value="VA">Ciudad del Vaticano</option>
						<option value="CO">Colombia</option>
						<option value="KM">Comoras</option>
						<option value="CG">Congo</option>
						<option value="CD">Congo, Rep&uacute;blica Democr&aacute;tica del</option>
						<option value="KP">Corea del Norte</option>
						<option value="KR">Corea del Sur</option>
						<option value="CI">Costa de Marfil</option>
						<option value="CR">Costa Rica</option>
						<option value="HR">Croacia</option>
						<option value="CU">Cuba</option>
						<option value="DK">Dinamarca</option>
						<option value="DM">Dominica</option>
						<option value="EC">Ecuador</option>
						<option value="EG">Egipto</option>
						<option value="SV">El Salvador</option>
						<option value="AE">Emiratos &Aacute;rabes Unidos</option>
						<option value="ER">Eritrea</option>
						<option value="SK">Eslovaquia</option>
						<option value="SI">Eslovenia</option>
						<option value="ES">Espa&ntilde;a</option>
						<option value="US">Estados Unidos</option>
						<option value="EE">Estonia</option>
						<option value="ET">Etiop&iacute;a</option>
						<option value="PH">Filipinas</option>
						<option value="FI">Finlandia</option>
						<option value="FJ">Fiyi</option>
						<option value="FR">Francia</option>
						<option value="GA">Gab&oacute;n</option>
						<option value="GM">Gambia</option>
						<option value="GE">Georgia</option>
						<option value="GH">Ghana</option>
						<option value="GI">Gibraltar</option>
						<option value="GD">Granada</option>
						<option value="GR">Grecia</option>
						<option value="GL">Groenlandia</option>
						<option value="GP">Guadalupe</option>
						<option value="GU">Guam</option>
						<option value="GT">Guatemala</option>
						<option value="GF">Guayana Francesa</option>
						<option value="GG">Guernsey</option>
						<option value="GN">Guinea</option>
						<option value="GW">Guinea-Bissau</option>
						<option value="GQ">Guinea Ecuatorial</option>
						<option value="GY">Guyana</option>
						<option value="HT">Hait&iacute;</option>
						<option value="HN">Honduras</option>
						<option value="HK">Hong Kong</option>
						<option value="HU">Hungr&iacute;a</option>
						<option value="IN">India</option>
						<option value="ID">Indonesia</option>
						<option value="IR">Ir&aacute;n</option>
						<option value="IQ">Iraq</option>
						<option value="IE">Irlanda</option>
						<option value="BV">Isla Bouvet</option>
						<option value="CX">Isla Christmas</option>
						<option value="IM">Isla de Man</option>
						<option value="IS">Islandia</option>
						<option value="NF">Isla Norfolk</option>
						<option value="AX">Islas Aland</option>
						<option value="KY">Islas Caim&aacute;n</option>
						<option value="CC">Islas Cocos</option>
						<option value="CK">Islas Cook</option>
						<option value="FO">Islas Feroe</option>
						<option value="GS">Islas Georgia del Sur y Sandwich del Sur</option>
						<option value="HM">Islas Heard y McDonald</option>
						<option value="FK">Islas Malvinas</option>
						<option value="MP">Islas Mariana del Norte</option>
						<option value="MH">Islas Marshall</option>
						<option value="UM">Islas menores alejadas de los Estados Unidos</option>
						<option value="PN">Islas Pitcairn</option>
						<option value="SB">Islas Salom&oacute;n</option>
						<option value="SJ">Islas Svalbard y Jan Mayen</option>
						<option value="TC">Islas Turcas y Caicos</option>
						<option value="VI">Islas V&iacute;rgenes, EE.UU.</option>
						<option value="VG">Islas V&iacute;rgenes Brit&aacute;nicas</option>
						<option value="IL">Israel</option>
						<option value="IT">Italia</option>
						<option value="JM">Jamaica</option>
						<option value="JP">Jap&oacute;n</option>
						<option value="JE">Jersey</option>
						<option value="JO">Jordania</option>
						<option value="KZ">Kazajist&aacute;n</option>
						<option value="KE">Kenia</option>
						<option value="KG">Kirguizist&aacute;n</option>
						<option value="KI">Kiribati</option>
						<option value="KW">Kuwait</option>
						<option value="LA">Laos</option>
						<option value="LS">Lesoto</option>
						<option value="LV">Letonia</option>
						<option value="LB">L&iacute;bano</option>
						<option value="LR">Liberia</option>
						<option value="LY">Libia</option>
						<option value="LI">Liechtenstein</option>
						<option value="LT">Lituania</option>
						<option value="LU">Luxemburgo</option>
						<option value="MO">Macao</option>
						<option value="MK">Macedonia</option>
						<option value="MG">Madagascar</option>
						<option value="MY">Malasia</option>
						<option value="MW">Malaui</option>
						<option value="MV">Maldivas</option>
						<option value="ML">Mali</option>
						<option value="MT">Malta</option>
						<option value="MA">Marruecos</option>
						<option value="MQ">Martinica</option>
						<option value="MU">Mauricio</option>
						<option value="MR">Mauritania</option>
						<option value="YT">Mayotte</option>
						<option value="MX">M&eacute;xico</option>
						<option value="FM">Micronesia</option>
						<option value="MD">Moldavia</option>
						<option value="MC">M&oacute;naco</option>
						<option value="MN">Mongolia</option>
						<option value="ME">Montenegro</option>
						<option value="MS">Montserrat</option>
						<option value="MZ">Mozambique</option>
						<option value="MM">Myanmar</option>
						<option value="NA">Namibia</option>
						<option value="NR">Nauru</option>
						<option value="NP">Nepal</option>
						<option value="NI">Nicaragua</option>
						<option value="NE">N&iacute;ger</option>
						<option value="NG">Nigeria</option>
						<option value="NU">Niue</option>
						<option value="NO">Noruega</option>
						<option value="NC">Nueva Caledonia</option>
						<option value="NZ">Nueva Zelanda</option>
						<option value="OM">Om&aacute;n</option>
						<option value="NL">Pa&iacute;ses Bajos</option>
						<option value="PK">Pakist&aacute;n</option>
						<option value="PW">Palaos</option>
						<option value="PA">Panam&aacute;</option>
						<option value="PG">Pap&uacute;a-Nueva Guinea</option>
						<option value="PY">Paraguay</option>
						<option value="PE">Per&uacute;</option>
						<option value="PF">Polinesia Francesa</option>
						<option value="PL">Polonia</option>
						<option value="PT">Portugal</option>
						<option value="PR">Puerto Rico</option>
						<option value="QA">Qatar</option>
						<option value="GB">Reino Unido</option>
						<option value="CF">Rep&uacute;blica Centroafricana</option>
						<option value="CZ">Rep&uacute;blica Checa</option>
						<option value="DO">Rep&uacute;blica Dominicana</option>
						<option value="RE">Reuni&oacute;n</option>
						<option value="RW">Ruanda</option>
						<option value="RO">Rumania</option>
						<option value="RU">Rusia</option>
						<option value="EH">S&aacute;hara Occidental</option>
						<option value="WS">Samoa</option>
						<option value="AS">Samoa americana</option>
						<option value="KN">San Crist&oacute;bal y Nieves</option>
						<option value="SM">San Marino</option>
						<option value="PM">San Pedro y Miquel&oacute;n</option>
						<option value="SH">Santa Elena</option>
						<option value="LC">Santa Luc&iacute;a</option>
						<option value="ST">Santo Tom&eacute; y Pr&iacute;ncipe</option>
						<option value="VC">San Vicente y las Granadinas</option>
						<option value="SN">Senegal</option>
						<option value="RS">Serbia</option>
						<option value="CS">Serbia y Montenegro</option>
						<option value="SC">Seychelles</option>
						<option value="SL">Sierra Leona</option>
						<option value="SG">Singapur</option>
						<option value="SY">Siria</option>
						<option value="SO">Somalia</option>
						<option value="LK">Sri Lanka</option>
						<option value="SZ">Suazilandia</option>
						<option value="ZA">Sud&aacute;frica</option>
						<option value="SD">Sud&aacute;n</option>
						<option value="SE">Suecia</option>
						<option value="CH">Suiza</option>
						<option value="SR">Surinam</option>
						<option value="TH">Tailandia</option>
						<option value="TW">Taiw&aacute;n</option>
						<option value="TZ">Tanzania</option>
						<option value="TJ">Tayikist&aacute;n</option>
						<option value="IO">Territorio Brit&aacute;nico del Oc&eacute;ano &Iacute;ndico</option>
						<option value="PS">Territorio Palestino</option>
						<option value="TF">Territorios Australes Franceses</option>
						<option value="TL">Timor Oriental</option>
						<option value="TG">Togo</option>
						<option value="TK">Tokelau</option>
						<option value="TO">Tonga</option>
						<option value="TT">Trinidad y Tobago</option>
						<option value="TN">T&uacute;nez</option>
						<option value="TM">Turkmenist&aacute;n</option>
						<option value="TR">Turqu&iacute;a</option>
						<option value="TV">Tuvalu</option>
						<option value="UA">Ucrania</option>
						<option value="UG">Uganda</option>
						<option value="UY">Uruguay</option>
						<option value="UZ">Uzbekist&aacute;n</option>
						<option value="VU">Vanuatu</option>
						<option value="VE">Venezuela</option>
						<option value="VN">Vietnam</option>
						<option value="WF">Wallis y Futuna</option>
						<option value="YE">Yemen</option>
						<option value="DJ">Yibuti</option>
						<option value="ZM">Zambia</option>
						<option value="ZW">Zimbabue</option>
					</select>
				</form>
			</td>
		</tr>
	</table>
<?php
	closetable();
?>